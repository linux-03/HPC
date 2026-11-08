// Checks for the SIMD types and the task manager.
// The SIMD part runs on whatever path the compiler flags select: generic, ARM NEON or AVX.
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <thread>
#include <sys/resource.h>

#include "simd.h"
#include "taskmanager.h"

using namespace ASC_HPC;

namespace {

int failures = 0;

void report(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what.c_str());
  if (!ok) failures++;
}

template <size_t S>
void checkArithmetic() {
  double a[S], b[S], c[S], out[S];
  for (size_t i = 0; i < S; i++) { a[i] = 1.5 + i; b[i] = 0.25 * (i + 2); c[i] = -0.5 * i; }
  SIMD<double, S> A(a), B(b), C(c);
  auto same = [&](auto expected, const char* op) {
    bool ok = true;
    for (size_t i = 0; i < S; i++) ok = ok && std::fabs(out[i] - expected(i)) <= 1e-15 * (1 + std::fabs(expected(i)));
    report(ok, std::string(op) + " for SIMD<double," + std::to_string(S) + ">");
  };
  (A + B).Store(out);   same([&](size_t i) { return a[i] + b[i]; }, "a+b");
  (A - B).Store(out);   same([&](size_t i) { return a[i] - b[i]; }, "a-b");
  (A * B).Store(out);   same([&](size_t i) { return a[i] * b[i]; }, "a*b");
  (A / B).Store(out);   same([&](size_t i) { return a[i] / b[i]; }, "a/b");
  (2.0 * B).Store(out); same([&](size_t i) { return 2.0 * b[i]; }, "2*b");
  (2.0 / B).Store(out); same([&](size_t i) { return 2.0 / b[i]; }, "2/b");
  FMA(A, B, C).Store(out); same([&](size_t i) { return a[i] * b[i] + c[i]; }, "fma");

  double sum = 0;
  for (size_t i = 0; i < S; i++) sum += a[i];
  report(std::fabs(HSum(A) - sum) < 1e-13, "horizontal sum for SIMD<double," + std::to_string(S) + ">");
  bool lanes = true;
  for (size_t i = 0; i < S; i++) lanes = lanes && A[i] == a[i];
  report(lanes, "lane access for SIMD<double," + std::to_string(S) + ">");
}

void checkMasksAndSelect() {
  double a[4] = {1, 2, 3, 4}, b[4] = {10, 20, 30, 40}, out[4] = {0, 0, 0, 0};
  SIMD<double, 4> A(a), B(b);
  auto mask = A >= SIMD<double, 4>(2.5);
  Select(mask, B, SIMD<double, 4>(-1.0)).Store(out);
  report(out[0] == -1 && out[1] == -1 && out[2] == 30 && out[3] == 40, "select with a >= mask");

  double stored[4] = {0, 0, 0, 0};
  A.Store(stored, mask);
  report(stored[0] == 0 && stored[1] == 0 && stored[2] == 3 && stored[3] == 4, "masked store");

  SIMD<double, 4> loaded(b, mask);
  loaded.Store(out);
  report(out[0] == 0 && out[1] == 0 && out[2] == 30 && out[3] == 40, "masked load");

  SIMD<double, 2> pair(a);
  report(pair.Lo()[0] == 1 && pair.Hi()[0] == 2, "Lo and Hi of SIMD<double,2>");
}

double cpuSeconds() {
  rusage u;
  getrusage(RUSAGE_SELF, &u);
  return u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec + u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
}

void checkTaskManager() {
  StartWorkers(3);

  std::atomic<long> sum{0};
  RunParallel(1000, [&](int nr, int) { sum += nr; });
  report(sum == 999L * 1000 / 2, "RunParallel runs every task once");

  bool caught = false;
  std::atomic<int> ran{0};
  try {
    RunParallel(50, [&](int nr, int) {
      ran++;
      if (nr == 17) throw std::runtime_error("task 17 failed");
    });
  } catch (const std::runtime_error&) { caught = true; }
  report(caught && ran == 50, "an exception in a task reaches the caller after all tasks ran");

  const double before = cpuSeconds();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  const double used = cpuSeconds() - before;
  report(used < 0.3, "idle workers stay idle (" + std::to_string(used) + " s CPU for 3 workers in 1 s)");

  StopWorkers();
}

}  // namespace

int main() {
  checkArithmetic<1>();
  checkArithmetic<2>();
  checkArithmetic<3>();
  checkArithmetic<4>();
  checkArithmetic<8>();
  checkArithmetic<12>();
  checkMasksAndSelect();
  checkTaskManager();
  return failures ? 1 : 0;
}
