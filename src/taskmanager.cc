#include <atomic>
#include <chrono>
#include <exception>
#include <mutex>
#include <thread>

#include <concurrentqueue.h>

#include "taskmanager.h"
#include "timer.h"


namespace ASC_HPC
{

  class Task
  {
  public:
    int nr, size;
    const std::function<void(int nr, int size)> * pfunc;
    std::atomic<int> * cnt;

    Task & operator++(int)
    {
      nr++;
      return *this;
    }
    Task & operator*() { return *this; }
  };

  
  typedef moodycamel::ConcurrentQueue<Task> TQueue; 
  typedef moodycamel::ProducerToken TPToken; 
  typedef moodycamel::ConsumerToken TCToken; 
  
  
  static std::atomic<bool> stop{false};
  static std::vector<std::thread> threads;
  static TQueue queue;
  
  // Idle workers spin briefly, then yield, then sleep, so they only use a core
  // while there is work. Spinning first keeps the start-up latency low.
  static void Backoff(int & idle)
  {
    idle++;
    if (idle < 1000) return;
    if (idle < 2000) { std::this_thread::yield(); return; }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }

  void StartWorkers(int num)
  {
    stop = false;
    for (int i = 0; i < num; i++)
      {
        TimeLine * patl = timeline.get();
        threads.push_back
          (std::thread([patl]()
          {
            if (patl)
              timeline = std::make_unique<TimeLine>();

            TPToken ptoken(queue);
            TCToken ctoken(queue);

            int idle = 0;
            while(true)
              {
                if (stop) break;

                Task task;
                if(!queue.try_dequeue_from_producer(ptoken, task))
                  if(!queue.try_dequeue(ctoken, task))
                    {
                      Backoff(idle);
                      continue;
                    }

                idle = 0;
                (*task.pfunc)(task.nr, task.size);
                (*task.cnt)++;
              }

            if (patl)
              patl -> AddTimeLine(std::move(*timeline));
          }));
      }
  }

  void StopWorkers()
  {
    stop = true;
    for (auto & t : threads)
      t.join();
    threads.clear();
  }


  void RunParallel (int num,
                    const std::function<void(int nr, int size)> & func)
  {
    TPToken ptoken(queue);
    TCToken ctoken(queue);

    std::atomic<int> cnt{0};

    // An exception in a task would otherwise terminate the program: keep the
    // first one and rethrow it here once every task has finished.
    std::exception_ptr error;
    std::mutex error_mutex;
    const std::function<void(int, int)> guarded = [&](int nr, int size)
    {
      try { func(nr, size); }
      catch (...)
        {
          std::lock_guard<std::mutex> lock(error_mutex);
          if (!error) error = std::current_exception();
        }
    };

    for (int i = 0; i < num; i++)
      {
        Task task;
        task.nr = i;
        task.size = num;
        task.pfunc = &guarded;
        task.cnt = &cnt;
        queue.enqueue(ptoken, task);
      }

    int idle = 0;
    while (cnt < num)
      {
        Task task;
        if(!queue.try_dequeue_from_producer(ptoken, task))
          if(!queue.try_dequeue(ctoken, task))
            {
              if (++idle > 1000) std::this_thread::yield();
              continue;
            }

        idle = 0;
        (*task.pfunc)(task.nr, task.size);
        (*task.cnt)++;
      }

    if (error)
      std::rethrow_exception(error);
  }
}
