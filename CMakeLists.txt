cmake_minimum_required(VERSION 3.20)
project(ASC_HPC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")


include_directories(src concurrentqueue)

find_package(Threads REQUIRED)
enable_testing()

add_executable(test_hpc tests/test_hpc.cc src/taskmanager.cc src/timer.cc)
target_link_libraries(test_hpc PRIVATE Threads::Threads)
add_test(NAME test_hpc COMMAND test_hpc)


add_executable (demo_tasks demos/demo_tasks.cc 
                src/taskmanager.cc src/timer.cc)
target_link_libraries(demo_tasks PRIVATE Threads::Threads)
                
target_sources (demo_tasks PUBLIC src/taskmanager.h src/timer.h)




add_executable (demo_simd demos/demo_simd.cc
                src/timer.cc)
                
target_sources (demo_simd PUBLIC src/simd.h src/timer.h)


add_executable (simd_timings demos/simd_timings.cc)
target_sources (simd_timings PUBLIC src/simd.h)


add_executable (timing_mem demos/timing_mem.cc)
target_sources (timing_mem PUBLIC src/simd.h)

