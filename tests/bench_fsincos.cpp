//
// Benchmark: x87 fsincos asm vs sin()+cos() from libm
//
// Compile (32-bit): g++ -O2 -m32 -o bench_fsincos bench_fsincos.cpp -lm
// Or cross:         i686-linux-gnu-g++ -O2 -o bench_fsincos bench_fsincos.cpp -lm
//

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

static const int ITERATIONS = 10000000;

// x87 fsincos instruction
static inline void asm_fsincos(double x, double &s, double &c)
{
   __asm__ ("fsincos;" : "=t" (c), "=u" (s) : "0" (x) : "st(7)");
}

// libm sin + cos (separate calls)
static inline void libm_sincos(double x, double &s, double &c)
{
   s = sin(x);
   c = cos(x);
}

// GNU extension sincos (single call, glibc optimized)
static inline void gnu_sincos(double x, double &s, double &c)
{
   ::sincos(x, &s, &c);
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
static volatile double sink_s, sink_c;

static double bench(const char *name, void (*fn)(double, double&, double&))
{
   double s, c;
   // Warmup
   for (int i = 0; i < 100000; i++)
   {
      fn(i * 0.0001, s, c);
      sink_s = s;
      sink_c = c;
   }

   double start = get_time_ns();
   for (int i = 0; i < ITERATIONS; i++)
   {
      fn(i * 0.0001, s, c);
      sink_s = s;
      sink_c = c;
   }
   double elapsed = get_time_ns() - start;
   double ns_per = elapsed / ITERATIONS;

   printf("  %-24s %8.2f ns/call  (%7.2f M calls/sec)\n",
          name, ns_per, 1000.0 / ns_per);

   return ns_per;
}

int main(void)
{
   printf("fsincos benchmark (%d iterations)\n\n", ITERATIONS);

   double t_asm = bench("x87 fsincos asm", asm_fsincos);
   double t_libm = bench("libm sin()+cos()", libm_sincos);
   double t_gnu = bench("glibc sincos()", gnu_sincos);

   printf("\n");
   printf("  x87 asm vs libm sin+cos:  %.1fx %s\n",
          t_asm < t_libm ? t_libm / t_asm : t_asm / t_libm,
          t_asm < t_libm ? "faster" : "slower");
   printf("  x87 asm vs glibc sincos:  %.1fx %s\n",
          t_asm < t_gnu ? t_gnu / t_asm : t_asm / t_gnu,
          t_asm < t_gnu ? "faster" : "slower");

   // Verify correctness
   double s1, c1, s2, c2;
   asm_fsincos(1.234, s1, c1);
   libm_sincos(1.234, s2, c2);
   printf("\n  correctness check (x=1.234):\n");
   printf("    asm:  sin=%.15f cos=%.15f\n", s1, c1);
   printf("    libm: sin=%.15f cos=%.15f\n", s2, c2);
   printf("    diff:     %.2e         %.2e\n", fabs(s1-s2), fabs(c1-c2));

   return 0;
}
