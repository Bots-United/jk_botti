//
// Benchmark: SSE polynomial fcos vs libm cos vs x87 fcos
//
// Compile (32-bit): g++ -O2 -m32 -o bench_fcos bench_fcos.cpp -lm
// Or cross:         i686-linux-gnu-g++ -O2 -o bench_fcos bench_fcos.cpp -lm
//

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static const int ITERATIONS = 10000000;

// SSE polynomial cos (same Cephes implementation as in util.cpp)
static inline double sse_fcos(double x)
{
   double abs_x = __builtin_fabs(x);

   double y = __builtin_floor(abs_x * (4.0 / M_PI));
   int j = (int)y;
   int odd = j & 1;
   j = (j + odd) & 7;
   y += odd;

   static const double DP1 = 7.85398125648498535156e-1;
   static const double DP2 = 3.77489470793079817668e-8;
   static const double DP3 = 2.69515142907905952645e-15;
   double z = ((abs_x - y * DP1) - y * DP2) - y * DP3;
   double zz = z * z;

   // Compute only the polynomial needed for this octant:
   //   j=0,4 (bit 1 clear): cos polynomial
   //   j=2,6 (bit 1 set):   sin polynomial
   double p;
   if ((j & 2) == 0)
   {
      p = 1.0 - 0.5 * zz + zz * zz * (4.16666666666665929218e-2 + zz *
         (-1.38888888888730564116e-3 + zz * (2.48015872888517045348e-5 + zz *
         (-2.75573141792967388112e-7 + zz * (2.08757008419747316778e-9 + zz *
         (-1.13585365213876817300e-11))))));
      if (j == 4) p = -p;
   }
   else
   {
      p = z + z * zz * (-1.66666666666666307295e-1 + zz *
         (8.33333333332211858878e-3 + zz * (-1.98412698295895385996e-4 + zz *
         (2.75573136213857245213e-6 + zz * (-2.50507477628578072866e-8 + zz *
         1.58962301576546568060e-10)))));
      if (j == 2) p = -p;
   }

   return p;
}

// x87 fcos instruction
static inline double x87_fcos(double x)
{
   double c;
   __asm__ ("fcos;" : "=t" (c) : "0" (x));
   return c;
}

// libm cos
static inline double libm_cos(double x)
{
   return cos(x);
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

static double bench(const char *name, double (*fn)(double))
{
   // Warmup
   for (int i = 0; i < 100000; i++)
   {
      sink = fn(i * 0.0001);
   }

   double start = get_time_ns();
   for (int i = 0; i < ITERATIONS; i++)
   {
      sink = fn(i * 0.0001);
   }
   double elapsed = get_time_ns() - start;
   double ns_per = elapsed / ITERATIONS;

   printf("  %-24s %8.2f ns/call  (%7.2f M calls/sec)\n",
          name, ns_per, 1000.0 / ns_per);

   return ns_per;
}

int main(void)
{
   printf("fcos benchmark (%d iterations)\n\n", ITERATIONS);

   double t_sse = bench("SSE polynomial fcos", sse_fcos);
   double t_x87 = bench("x87 fcos", x87_fcos);
   double t_libm = bench("libm cos()", libm_cos);

   printf("\n");
   printf("  SSE poly vs libm cos:     %.1fx %s\n",
          t_sse < t_libm ? t_libm / t_sse : t_sse / t_libm,
          t_sse < t_libm ? "faster" : "slower");
   printf("  SSE poly vs x87 fcos:  %.1fx %s\n",
          t_sse < t_x87 ? t_x87 / t_sse : t_sse / t_x87,
          t_sse < t_x87 ? "faster" : "slower");
   printf("  x87 fcos vs libm cos:  %.1fx %s\n",
          t_x87 < t_libm ? t_libm / t_x87 : t_x87 / t_libm,
          t_x87 < t_libm ? "faster" : "slower");

   // Verify correctness
   double c_sse = sse_fcos(1.234);
   double c_x87 = x87_fcos(1.234);
   double c_libm = libm_cos(1.234);
   printf("\n  correctness check (x=1.234):\n");
   printf("    SSE poly: cos=%.15f\n", c_sse);
   printf("    x87:      cos=%.15f\n", c_x87);
   printf("    libm:     cos=%.15f\n", c_libm);
   printf("    diff SSE-libm: %.2e\n", fabs(c_sse - c_libm));
   printf("    diff x87-libm: %.2e\n", fabs(c_x87 - c_libm));

   return 0;
}
