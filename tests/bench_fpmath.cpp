//
// Benchmark: x87 vs SSE floating-point math for bot operations
//
// Build via tests/Makefile: make bench_fpmath
// Or manually (both variants):
//   i686-linux-gnu-g++ -O2 -march=i686 -msse -msse2 -msse3 -o bench_fpmath_x87 bench_fpmath.cpp -lm
//   i686-linux-gnu-g++ -O2 -march=i686 -msse -msse2 -msse3 -mfpmath=sse -o bench_fpmath_sse bench_fpmath.cpp -lm
//

#include <stdio.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#include "engine_mock.h"

// Pull in util.cpp to get real fsincos, UTIL_AnglesToForward, etc.
#include "../util.cpp"

// Stubs for globals referenced by util.cpp
bot_t bots[32];
qboolean is_team_play;

// Volatile sinks to prevent optimization
static volatile float sink_f;
static volatile double sink_d;

static void sink_vec(const Vector &v)
{
   sink_f = v.x;
   sink_f = v.y;
   sink_f = v.z;
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

typedef void (*bench_fn)(int);

static double bench(const char *name, bench_fn fn, int iterations)
{
   // Warmup
   fn(iterations / 10);

   double start = get_time_ns();
   fn(iterations);
   double elapsed = get_time_ns() - start;
   double ns_per = elapsed / iterations;

   printf("  %-34s %8.2f ns/call  (%7.2f M calls/sec)\n",
          name, ns_per, 1000.0 / ns_per);
   return ns_per;
}

// --- Benchmark functions ---

static void bench_vec_length(int n)
{
   for (int i = 0; i < n; i++) {
      Vector v(i * 0.01f, i * 0.02f, i * 0.03f);
      sink_d = v.Length();
   }
}

static void bench_vec_normalize(int n)
{
   for (int i = 0; i < n; i++) {
      Vector v(i * 0.01f + 1.0f, i * 0.02f + 2.0f, i * 0.03f + 3.0f);
      sink_vec(v.Normalize());
   }
}

static void bench_vec_dotproduct(int n)
{
   Vector a(1.0f, 2.0f, 3.0f);
   for (int i = 0; i < n; i++) {
      Vector b(i * 0.01f, i * 0.02f, i * 0.03f);
      sink_f = DotProduct(a, b);
   }
}

static void bench_angles_to_forward(int n)
{
   for (int i = 0; i < n; i++) {
      Vector angles(i * 0.1f, i * 0.2f, 0);
      sink_vec(UTIL_AnglesToForward(angles));
   }
}

static void bench_make_vectors(int n)
{
   Vector fwd, right, up;
   for (int i = 0; i < n; i++) {
      Vector angles(i * 0.1f, i * 0.2f, i * 0.05f);
      UTIL_MakeVectorsPrivate(angles, fwd, right, up);
      sink_vec(fwd);
   }
}

static void bench_vec_to_angles(int n)
{
   for (int i = 0; i < n; i++) {
      Vector v(i * 0.01f + 1.0f, i * 0.02f + 0.5f, i * 0.03f - 0.3f);
      sink_vec(UTIL_VecToAngles(v));
   }
}

static void bench_distance_calc(int n)
{
   // Simulates waypoint distance comparison (common in pathfinding)
   Vector origin(100.0f, 200.0f, 50.0f);
   for (int i = 0; i < n; i++) {
      Vector wpt(i * 1.5f, i * 0.8f, i * 0.1f);
      Vector diff = wpt - origin;
      sink_d = diff.Length();
   }
}

// Combined: simulate one bot think frame of navigation math
static void bench_combined_navframe(int n)
{
   Vector origin(100.0f, 200.0f, 50.0f);
   Vector angles(10.0f, 45.0f, 0.0f);
   for (int i = 0; i < n; i++) {
      // AnglesToForward for movement direction
      Vector fwd = UTIL_AnglesToForward(angles);

      // Distance to 3 waypoints
      Vector w1(i*1.0f, i*2.0f, 50.0f);
      Vector w2(i*1.5f, i*0.5f, 55.0f);
      Vector w3(i*0.8f, i*1.2f, 48.0f);
      double d1 = (w1 - origin).Length();
      double d2 = (w2 - origin).Length();
      double d3 = (w3 - origin).Length();

      // Normalize direction to closest
      Vector dir = (d1 < d2 && d1 < d3) ? w1 - origin :
                   (d2 < d3) ? w2 - origin : w3 - origin;
      Vector norm_dir = dir.Normalize();

      // Dot product for angle check
      float dot = DotProduct(fwd, norm_dir);

      sink_f = dot;
      sink_d = d1;
      angles.y += 0.1f;
   }
}

int main(void)
{
   const int N = 5000000;

   printf("FP math benchmark (%d iterations per test)\n", N);

#ifdef __SSE_MATH__
   printf("Mode: SSE (-mfpmath=sse)\n\n");
#else
   printf("Mode: x87 (default)\n\n");
#endif

   bench("Vector::Length()",             bench_vec_length,        N);
   bench("Vector::Normalize()",          bench_vec_normalize,     N);
   bench("DotProduct()",                 bench_vec_dotproduct,    N);
   bench("AnglesToForward() [2x fsincos]", bench_angles_to_forward, N);
   bench("MakeVectors() [3x fsincos]",   bench_make_vectors,      N);
   bench("VecToAngles() [atan2+sqrt]",   bench_vec_to_angles,     N);
   bench("Distance calc (sub+length)",   bench_distance_calc,     N);
   bench("Combined nav frame",           bench_combined_navframe, N);

   return 0;
}
