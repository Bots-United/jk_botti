//
// Benchmark: atan2 implementations
//
// Compile (32-bit): g++ -O2 -m32 -o bench_atan2 bench_atan2.cpp -lm
// Or cross:         i686-linux-gnu-g++ -O2 -o bench_atan2 bench_atan2.cpp -lm
//

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static const int ITERATIONS = 10000000;

// libm atan2
static inline double libm_atan2(double y, double x)
{
   return atan2(y, x);
}

// x87 fpatan instruction
static inline double x87_atan2(double y, double x)
{
   double r;
   __asm__ ("fpatan;" : "=t" (r) : "0" (x), "u" (y) : "st(1)");
   return r;
}

// GNU extension: atan2 via sincos + atan (for comparison)
// Actually just test __builtin_atan2 to see if GCC does anything special
static inline double builtin_atan2(double y, double x)
{
   return __builtin_atan2(y, x);
}

static double get_time_ns(void)
{
#ifdef _WIN32
   LARGE_INTEGER count, freq;
   QueryPerformanceFrequency(&freq);
   QueryPerformanceCounter(&count);
   return (double)count.QuadPart / (double)freq.QuadPart * 1e9;
#else
   struct timespec ts;
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
}

// Volatile to prevent optimizing away results
static volatile double sink;

static double bench(const char *name, double (*fn)(double, double))
{
   // Warmup with varied inputs across all quadrants
   for (int i = 0; i < 100000; i++)
   {
      double y = (i - 50000) * 0.01;
      double x = (i * 7 - 50000) * 0.01;
      sink = fn(y, x);
   }

   double start = get_time_ns();
   for (int i = 0; i < ITERATIONS; i++)
   {
      // Vary both y and x to cover all quadrants and signs
      double y = (i - ITERATIONS/2) * 0.0001;
      double x = ((i * 7) % ITERATIONS - ITERATIONS/2) * 0.0001;
      sink = fn(y, x);
   }
   double elapsed = get_time_ns() - start;
   double ns_per = elapsed / ITERATIONS;

   printf("  %-24s %8.2f ns/call  (%7.2f M calls/sec)\n",
          name, ns_per, 1000.0 / ns_per);

   return ns_per;
}

int main(void)
{
   printf("atan2 benchmark (%d iterations)\n\n", ITERATIONS);

   double t_libm = bench("libm atan2()", libm_atan2);
   double t_x87 = bench("x87 fpatan", x87_atan2);
   double t_builtin = bench("__builtin_atan2()", builtin_atan2);

   printf("\n");
   printf("  x87 fpatan vs libm:       %.1fx %s\n",
          t_x87 < t_libm ? t_libm / t_x87 : t_x87 / t_libm,
          t_x87 < t_libm ? "faster" : "slower");
   printf("  __builtin vs libm:        %.1fx %s\n",
          t_builtin < t_libm ? t_libm / t_builtin : t_builtin / t_libm,
          t_builtin < t_libm ? "faster" : "slower");

   // Verify correctness
   double test_cases[][2] = {
      {1.0, 1.0}, {-1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0},
      {0.0, 1.0}, {1.0, 0.0}, {0.0, -1.0}, {-1.0, 0.0},
      {0.5, 0.866}, {100.0, 200.0}
   };
   int ncases = sizeof(test_cases) / sizeof(test_cases[0]);

   printf("\n  correctness check:\n");
   printf("    %-14s %-20s %-20s %-10s\n", "y, x", "libm", "x87 fpatan", "diff");
   double max_diff = 0;
   for (int i = 0; i < ncases; i++)
   {
      double y = test_cases[i][0], x = test_cases[i][1];
      double r_libm = libm_atan2(y, x);
      double r_x87 = x87_atan2(y, x);
      double diff = fabs(r_libm - r_x87);
      if (diff > max_diff) max_diff = diff;
      printf("    (%5.1f,%6.3f) %20.15f %20.15f %.2e\n",
             y, x, r_libm, r_x87, diff);
   }
   printf("    max diff: %.2e\n", max_diff);

   return 0;
}
