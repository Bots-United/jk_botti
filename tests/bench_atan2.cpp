//
// Benchmark: SSE polynomial atan2 vs x87 fpatan vs libm atan2
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

// SSE polynomial atan (same Cephes implementation as in util.cpp)
static inline double sse_atan(double x)
{
   double abs_x = __builtin_fabs(x);

   static const double T3P8 = 2.41421356237309504880;
   static const double MOREBITS = 6.123233995736765886130e-17;

   double z, base, morebits_scale;
   if (abs_x > T3P8)
   {
      z = -1.0 / abs_x;
      base = M_PI_2;
      morebits_scale = 1.0;
   }
   else if (abs_x > 0.66)
   {
      z = (abs_x - 1.0) / (abs_x + 1.0);
      base = M_PI_4;
      morebits_scale = 0.5;
   }
   else
   {
      z = abs_x;
      base = 0.0;
      morebits_scale = 0.0;
   }

   double zz = z * z;

   double p = ((((-8.750608600031904122785e-1 * zz
      - 1.615753718733365076637e1) * zz
      - 7.500855792314704667340e1) * zz
      - 1.228866684490136173410e2) * zz
      - 6.485021904942025371773e1) * zz;

   double q = ((((zz
      + 2.485846490142306297962e1) * zz
      + 1.650270098316988542046e2) * zz
      + 4.328810604912902668951e2) * zz
      + 4.853903996359136964868e2) * zz
      + 1.945506571482613964425e2;

   z = z + z * (p / q) + morebits_scale * MOREBITS + base;

   if (x < 0)
      z = -z;

   return z;
}

// SSE polynomial atan2
static inline double sse_atan2(double y, double x)
{
   if (y == 0.0)
   {
      if (x >= 0.0)
         return y;
      else
         return __builtin_copysign(M_PI, y);
   }
   if (x == 0.0)
      return (y > 0.0) ? M_PI_2 : -M_PI_2;

   double r = sse_atan(y / x);

   if (x < 0.0)
   {
      if (y >= 0.0)
         r += M_PI;
      else
         r -= M_PI;
   }

   return r;
}

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

   double t_sse = bench("SSE polynomial atan2", sse_atan2);
   double t_x87 = bench("x87 fpatan", x87_atan2);
   double t_libm = bench("libm atan2()", libm_atan2);

   printf("\n");
   printf("  SSE poly vs libm atan2:   %.1fx %s\n",
          t_sse < t_libm ? t_libm / t_sse : t_sse / t_libm,
          t_sse < t_libm ? "faster" : "slower");
   printf("  SSE poly vs x87 fpatan:   %.1fx %s\n",
          t_sse < t_x87 ? t_x87 / t_sse : t_sse / t_x87,
          t_sse < t_x87 ? "faster" : "slower");
   printf("  x87 fpatan vs libm atan2: %.1fx %s\n",
          t_x87 < t_libm ? t_libm / t_x87 : t_x87 / t_libm,
          t_x87 < t_libm ? "faster" : "slower");

   // Verify correctness
   double test_cases[][2] = {
      {1.0, 1.0}, {-1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0},
      {0.0, 1.0}, {1.0, 0.0}, {0.0, -1.0}, {-1.0, 0.0},
      {0.5, 0.866}, {100.0, 200.0}
   };
   int ncases = sizeof(test_cases) / sizeof(test_cases[0]);

   printf("\n  correctness check:\n");
   printf("    %-14s %-20s %-20s %-20s\n", "y, x", "SSE poly", "libm", "diff");
   double max_diff = 0;
   for (int i = 0; i < ncases; i++)
   {
      double y = test_cases[i][0], x = test_cases[i][1];
      double r_sse = sse_atan2(y, x);
      double r_libm = libm_atan2(y, x);
      double diff = fabs(r_sse - r_libm);
      if (diff > max_diff) max_diff = diff;
      printf("    (%5.1f,%6.3f) %20.15f %20.15f %.2e\n",
             y, x, r_sse, r_libm, diff);
   }
   printf("    max diff: %.2e\n", max_diff);

   return 0;
}
