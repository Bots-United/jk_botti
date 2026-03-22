//
// JK_Botti - unit tests for fsincos implementations
//
// Uses #include-the-.cpp approach to access inline functions directly.
//

#include <math.h>

#include "engine_mock.h"
#include "test_common.h"

// Include util.cpp to access fsincos_sse, fsincos_x87, fsincos
#include "../util.cpp"

// Stubs for globals referenced by util.cpp
bot_t bots[32];
qboolean is_team_play;

#define ASSERT_DOUBLE_NEAR(actual, expected, eps) do { \
   double _a = (actual), _e = (expected), _eps = (eps); \
   if (fabs(_a - _e) > _eps) { \
      printf("FAIL\n    expected: %.15e (+/- %.2e)\n    got:      %.15e\n    diff:     %.2e\n", \
             _e, _eps, _a, fabs(_a - _e)); \
      return 1; \
   } \
} while(0)

// ============================================================
// fsincos_x87 tests (lightweight, verify against libm)
// ============================================================

static int test_fsincos_x87(void)
{
   double s, c;
   double eps = 1e-15;

   printf("fsincos_x87:\n");

   TEST("x=0");
   fsincos_x87(0.0, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(0.0), eps);
   ASSERT_DOUBLE_NEAR(c, cos(0.0), eps);
   PASS();

   TEST("x=pi/4");
   fsincos_x87(M_PI/4, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(M_PI/4), eps);
   ASSERT_DOUBLE_NEAR(c, cos(M_PI/4), eps);
   PASS();

   TEST("x=pi/2");
   fsincos_x87(M_PI/2, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(M_PI/2), eps);
   ASSERT_DOUBLE_NEAR(c, cos(M_PI/2), eps);
   PASS();

   TEST("x=pi");
   fsincos_x87(M_PI, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(M_PI), eps);
   ASSERT_DOUBLE_NEAR(c, cos(M_PI), eps);
   PASS();

   TEST("x=-pi/3");
   fsincos_x87(-M_PI/3, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(-M_PI/3), eps);
   ASSERT_DOUBLE_NEAR(c, cos(-M_PI/3), eps);
   PASS();

   TEST("x=2*pi (full circle)");
   fsincos_x87(2*M_PI, s, c);
   ASSERT_DOUBLE_NEAR(s, sin(2*M_PI), eps);
   ASSERT_DOUBLE_NEAR(c, cos(2*M_PI), eps);
   PASS();

   // x87 NaN/inf not tested here: fsincos is an x87 hardware instruction,
   // and valgrind's emulation returns wrong results for inf inputs
   // (cos(inf)=inf instead of NaN). The SSE polynomial handles these
   // correctly and is tested in test_fsincos_sse_special_values().

   return 0;
}

// ============================================================
// fsincos_sse tests (comprehensive, verify against fsincos_x87)
// ============================================================

static int test_fsincos_sse_special_angles(void)
{
   double s_sse, c_sse, s_x87, c_x87;
   double eps = 2.5e-16; // ~1 ULP for double

   printf("fsincos_sse special angles:\n");

   TEST("x=0");
   fsincos_sse(0.0, s_sse, c_sse);
   fsincos_x87(0.0, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=pi/6 (30 deg)");
   fsincos_sse(M_PI/6, s_sse, c_sse);
   fsincos_x87(M_PI/6, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=pi/4 (45 deg)");
   fsincos_sse(M_PI/4, s_sse, c_sse);
   fsincos_x87(M_PI/4, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=pi/3 (60 deg)");
   fsincos_sse(M_PI/3, s_sse, c_sse);
   fsincos_x87(M_PI/3, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=pi/2 (90 deg)");
   fsincos_sse(M_PI/2, s_sse, c_sse);
   fsincos_x87(M_PI/2, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=pi (180 deg)");
   fsincos_sse(M_PI, s_sse, c_sse);
   fsincos_x87(M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=3*pi/2 (270 deg)");
   fsincos_sse(3*M_PI/2, s_sse, c_sse);
   fsincos_x87(3*M_PI/2, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=2*pi (360 deg)");
   fsincos_sse(2*M_PI, s_sse, c_sse);
   fsincos_x87(2*M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   return 0;
}

static int test_fsincos_sse_negative_angles(void)
{
   double s_sse, c_sse, s_x87, c_x87;
   double eps = 2.5e-16;

   printf("fsincos_sse negative angles:\n");

   TEST("x=-pi/4 (-45 deg)");
   fsincos_sse(-M_PI/4, s_sse, c_sse);
   fsincos_x87(-M_PI/4, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=-pi/2 (-90 deg)");
   fsincos_sse(-M_PI/2, s_sse, c_sse);
   fsincos_x87(-M_PI/2, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=-pi (-180 deg)");
   fsincos_sse(-M_PI, s_sse, c_sse);
   fsincos_x87(-M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   TEST("x=-2*pi (-360 deg)");
   fsincos_sse(-2*M_PI, s_sse, c_sse);
   fsincos_x87(-2*M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, eps);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, eps);
   PASS();

   return 0;
}

static int test_fsincos_sse_bot_angles(void)
{
   double s_sse, c_sse, s_x87, c_x87;
   double eps = 2.5e-16;

   printf("fsincos_sse bot angle range (degrees -> radians):\n");

   // Bot angles are degrees converted via: angle * (M_PI*2 / 360)
   // Test all 360 degrees in 1-degree steps
   TEST("sweep -180 to +180 deg (1 deg steps)");
   for (int deg = -180; deg <= 180; deg++)
   {
      double x = deg * (M_PI * 2 / 360);
      fsincos_sse(x, s_sse, c_sse);
      fsincos_x87(x, s_x87, c_x87);
      if (fabs(s_sse - s_x87) > eps || fabs(c_sse - c_x87) > eps)
      {
         printf("FAIL at deg=%d (x=%.6f)\n"
                "    sin: sse=%.15e x87=%.15e diff=%.2e\n"
                "    cos: sse=%.15e x87=%.15e diff=%.2e\n",
                deg, x, s_sse, s_x87, fabs(s_sse - s_x87),
                c_sse, c_x87, fabs(c_sse - c_x87));
         return 1;
      }
   }
   PASS();

   // Fine-grained sweep at 0.1 degree steps
   TEST("sweep -360 to +360 deg (0.1 deg steps)");
   for (int i = -3600; i <= 3600; i++)
   {
      double x = (i * 0.1) * (M_PI * 2 / 360);
      fsincos_sse(x, s_sse, c_sse);
      fsincos_x87(x, s_x87, c_x87);
      if (fabs(s_sse - s_x87) > eps || fabs(c_sse - c_x87) > eps)
      {
         printf("FAIL at deg=%.1f (x=%.6f)\n"
                "    sin: sse=%.15e x87=%.15e diff=%.2e\n"
                "    cos: sse=%.15e x87=%.15e diff=%.2e\n",
                i * 0.1, x, s_sse, s_x87, fabs(s_sse - s_x87),
                c_sse, c_x87, fabs(c_sse - c_x87));
         return 1;
      }
   }
   PASS();

   return 0;
}

static int test_fsincos_sse_octant_boundaries(void)
{
   double s_sse, c_sse, s_x87, c_x87;
   double eps = 2.5e-16;

   printf("fsincos_sse octant boundaries:\n");

   // Test values near pi/4 boundaries where octant selection switches
   double boundaries[] = {
      M_PI/4 - 1e-10, M_PI/4, M_PI/4 + 1e-10,
      M_PI/2 - 1e-10, M_PI/2, M_PI/2 + 1e-10,
      3*M_PI/4 - 1e-10, 3*M_PI/4, 3*M_PI/4 + 1e-10,
      M_PI - 1e-10, M_PI, M_PI + 1e-10,
   };

   TEST("near pi/4 multiples (+/- 1e-10)");
   for (int i = 0; i < (int)(sizeof(boundaries)/sizeof(boundaries[0])); i++)
   {
      double x = boundaries[i];
      fsincos_sse(x, s_sse, c_sse);
      fsincos_x87(x, s_x87, c_x87);
      if (fabs(s_sse - s_x87) > eps || fabs(c_sse - c_x87) > eps)
      {
         printf("FAIL at x=%.15e\n"
                "    sin: sse=%.15e x87=%.15e diff=%.2e\n"
                "    cos: sse=%.15e x87=%.15e diff=%.2e\n",
                x, s_sse, s_x87, fabs(s_sse - s_x87),
                c_sse, c_x87, fabs(c_sse - c_x87));
         return 1;
      }
   }
   PASS();

   // Same boundaries but negative
   TEST("negative near pi/4 multiples");
   for (int i = 0; i < (int)(sizeof(boundaries)/sizeof(boundaries[0])); i++)
   {
      double x = -boundaries[i];
      fsincos_sse(x, s_sse, c_sse);
      fsincos_x87(x, s_x87, c_x87);
      if (fabs(s_sse - s_x87) > eps || fabs(c_sse - c_x87) > eps)
      {
         printf("FAIL at x=%.15e\n"
                "    sin: sse=%.15e x87=%.15e diff=%.2e\n"
                "    cos: sse=%.15e x87=%.15e diff=%.2e\n",
                x, s_sse, s_x87, fabs(s_sse - s_x87),
                c_sse, c_x87, fabs(c_sse - c_x87));
         return 1;
      }
   }
   PASS();

   return 0;
}

static int test_fsincos_sse_max_error(void)
{
   double s_sse, c_sse, s_x87, c_x87;

   printf("fsincos_sse max error:\n");

   // Sweep a large range at fine granularity
   TEST("max error across 10M points in [-5*pi, 5*pi]");
   double max_sin_err = 0, max_cos_err = 0;
   int n = 10000000;
   for (int i = 0; i < n; i++)
   {
      double x = (i - n/2) * (10.0 * M_PI / n);
      fsincos_sse(x, s_sse, c_sse);
      fsincos_x87(x, s_x87, c_x87);
      double se = fabs(s_sse - s_x87);
      double ce = fabs(c_sse - c_x87);
      if (se > max_sin_err)
         max_sin_err = se;
      if (ce > max_cos_err)
         max_cos_err = ce;
   }
   // Both should be within ~1 ULP of double precision
   if (max_sin_err > 2.5e-16 || max_cos_err > 2.5e-16)
   {
      printf("FAIL\n    max sin error: %.2e\n    max cos error: %.2e\n",
             max_sin_err, max_cos_err);
      return 1;
   }
   PASS();

   return 0;
}

static int test_fsincos_sse_identity(void)
{
   double s, c;

   printf("fsincos_sse identities:\n");

   // sin^2 + cos^2 = 1
   TEST("sin^2 + cos^2 = 1 across bot angle range");
   for (int deg = -360; deg <= 360; deg++)
   {
      double x = deg * (M_PI * 2 / 360);
      fsincos_sse(x, s, c);
      double sum = s*s + c*c;
      if (fabs(sum - 1.0) > 1e-14)
      {
         printf("FAIL at deg=%d: sin^2+cos^2 = %.15e\n", deg, sum);
         return 1;
      }
   }
   PASS();

   // sin(-x) = -sin(x), cos(-x) = cos(x)
   TEST("sin(-x) = -sin(x), cos(-x) = cos(x)");
   for (int deg = 1; deg <= 360; deg++)
   {
      double x = deg * (M_PI * 2 / 360);
      double sp, cp, sn, cn;
      fsincos_sse(x, sp, cp);
      fsincos_sse(-x, sn, cn);
      if (fabs(sn + sp) > 2.5e-16 || fabs(cn - cp) > 2.5e-16)
      {
         printf("FAIL at deg=%d\n"
                "    sin(%d)=%.15e sin(%d)=%.15e sum=%.2e\n"
                "    cos(%d)=%.15e cos(%d)=%.15e diff=%.2e\n",
                deg, deg, sp, -deg, sn, fabs(sn + sp),
                deg, cp, -deg, cn, fabs(cn - cp));
         return 1;
      }
   }
   PASS();

   return 0;
}

static int test_fsincos_sse_special_values(void)
{
   double s_sse, c_sse;

   printf("fsincos_sse special values:\n");

   TEST("x=NaN -> NaN");
   fsincos_sse(__builtin_nan(""), s_sse, c_sse);
   ASSERT_TRUE(isnan(s_sse));
   ASSERT_TRUE(isnan(c_sse));
   PASS();

   TEST("x=+inf -> NaN");
   fsincos_sse(__builtin_inf(), s_sse, c_sse);
   ASSERT_TRUE(isnan(s_sse));
   ASSERT_TRUE(isnan(c_sse));
   PASS();

   TEST("x=-inf -> NaN");
   fsincos_sse(-__builtin_inf(), s_sse, c_sse);
   ASSERT_TRUE(isnan(s_sse));
   ASSERT_TRUE(isnan(c_sse));
   PASS();

   // Large values: range reduction precision degrades with magnitude.
   // Bot angles never exceed ~2*pi, but verify reasonable large values work.
   double s_x87, c_x87;

   TEST("x=100*pi (large, still precise)");
   fsincos_sse(100*M_PI, s_sse, c_sse);
   fsincos_x87(100*M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, 1e-14);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, 1e-14);
   PASS();

   TEST("x=-1000*pi");
   fsincos_sse(-1000*M_PI, s_sse, c_sse);
   fsincos_x87(-1000*M_PI, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, 1e-13);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, 1e-13);
   PASS();

   TEST("x=1e6 (precision ~1e-15)");
   fsincos_sse(1e6, s_sse, c_sse);
   fsincos_x87(1e6, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, 1e-12);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, 1e-12);
   PASS();

   TEST("x=-1e6");
   fsincos_sse(-1e6, s_sse, c_sse);
   fsincos_x87(-1e6, s_x87, c_x87);
   ASSERT_DOUBLE_NEAR(s_sse, s_x87, 1e-12);
   ASSERT_DOUBLE_NEAR(c_sse, c_x87, 1e-12);
   PASS();

   return 0;
}

// ============================================================
// fcos_sse tests (verify against libm cos)
// ============================================================

static int test_fcos_sse_special_angles(void)
{
   printf("fcos_sse special angles:\n");

   struct { const char *name; double x; } cases[] = {
      {"0",       0.0},
      {"pi/6",    M_PI/6},
      {"pi/4",    M_PI/4},
      {"pi/3",    M_PI/3},
      {"pi/2",    M_PI/2},
      {"pi",      M_PI},
      {"3pi/2",   3*M_PI/2},
      {"2pi",     2*M_PI},
      {"-pi/4",   -M_PI/4},
      {"-pi/2",   -M_PI/2},
      {"-pi",     -M_PI},
      {"-2pi",    -2*M_PI},
   };
   int n = sizeof(cases) / sizeof(cases[0]);

   for (int i = 0; i < n; i++)
   {
      TEST(cases[i].name);
      double got = fcos_sse(cases[i].x);
      double expected = cos(cases[i].x);
      ASSERT_DOUBLE_NEAR(got, expected, 2e-16);
      PASS();
   }

   return 0;
}

static int test_fcos_sse_bot_angles(void)
{
   printf("fcos_sse bot angle range (degrees -> radians):\n");

   TEST("sweep -180 to +180 degrees (1-degree steps)");
   double max_err = 0;
   for (int deg = -180; deg <= 180; deg++)
   {
      double x = deg * (M_PI / 180.0);
      double got = fcos_sse(x);
      double expected = cos(x);
      double err = fabs(got - expected);
      if (err > max_err) max_err = err;
      ASSERT_DOUBLE_NEAR(got, expected, 2e-16);
   }
   printf("    max error: %.2e\n", max_err);
   PASS();

   TEST("fine sweep 0 to 2pi (0.01 radian steps)");
   max_err = 0;
   for (int i = 0; i <= 628; i++)
   {
      double x = i * 0.01;
      double got = fcos_sse(x);
      double expected = cos(x);
      double err = fabs(got - expected);
      if (err > max_err) max_err = err;
      ASSERT_DOUBLE_NEAR(got, expected, 2e-16);
   }
   printf("    max error: %.2e\n", max_err);
   PASS();

   return 0;
}

static int test_fcos_sse_consistency_with_fsincos(void)
{
   double s, c;

   printf("fcos_sse consistency with fsincos_sse:\n");

   TEST("fcos_sse matches fsincos_sse cos output across full range");
   double max_err = 0;
   for (int deg = -360; deg <= 360; deg++)
   {
      double x = deg * (M_PI / 180.0);
      double got = fcos_sse(x);
      fsincos_sse(x, s, c);
      double err = fabs(got - c);
      if (err > max_err) max_err = err;
      if (err > 0)
      {
         printf("FAIL at %d deg: fcos=%.15e fsincos.c=%.15e diff=%.2e\n",
                deg, got, c, err);
         return 1;
      }
   }
   printf("    max error: %.2e (expect exact match)\n", max_err);
   PASS();

   return 0;
}

static int test_fcos_sse_special_values(void)
{
   printf("fcos_sse special values:\n");

   TEST("x=NaN -> NaN");
   ASSERT_TRUE(isnan(fcos_sse(__builtin_nan(""))));
   PASS();

   TEST("x=+inf -> NaN");
   ASSERT_TRUE(isnan(fcos_sse(__builtin_inf())));
   PASS();

   TEST("x=-inf -> NaN");
   ASSERT_TRUE(isnan(fcos_sse(-__builtin_inf())));
   PASS();

   TEST("cos is even: fcos(x) == fcos(-x)");
   double angles[] = {0.1, 0.5, 1.0, M_PI/3, M_PI, 2*M_PI, 5.5};
   for (int i = 0; i < (int)(sizeof(angles)/sizeof(angles[0])); i++)
   {
      double pos = fcos_sse(angles[i]);
      double neg = fcos_sse(-angles[i]);
      if (pos != neg)
      {
         printf("FAIL: fcos(%.3f)=%.15e != fcos(-%.3f)=%.15e\n",
                angles[i], pos, angles[i], neg);
         return 1;
      }
   }
   PASS();

   return 0;
}

static int test_fcos_runtime(void)
{
   printf("fcos_runtime (dispatch wrapper):\n");

   TEST("matches libm cos across bot angle range");
   double max_err = 0;
   for (int deg = -180; deg <= 180; deg++)
   {
      double x = deg * (M_PI / 180.0);
      double got = fcos_runtime(x);
      double expected = cos(x);
      double err = fabs(got - expected);
      if (err > max_err) max_err = err;
      ASSERT_DOUBLE_NEAR(got, expected, 2e-16);
   }
   printf("    max error: %.2e\n", max_err);
   PASS();

   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   fail |= test_fsincos_x87();
   fail |= test_fsincos_sse_special_angles();
   fail |= test_fsincos_sse_negative_angles();
   fail |= test_fsincos_sse_bot_angles();
   fail |= test_fsincos_sse_octant_boundaries();
   fail |= test_fsincos_sse_max_error();
   fail |= test_fsincos_sse_identity();
   fail |= test_fsincos_sse_special_values();

   fail |= test_fcos_sse_special_angles();
   fail |= test_fcos_sse_bot_angles();
   fail |= test_fcos_sse_consistency_with_fsincos();
   fail |= test_fcos_sse_special_values();
   fail |= test_fcos_runtime();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
   if (tests_passed == tests_run)
      printf("All tests passed.\n");
   else
      printf("SOME TESTS FAILED!\n");

   return (tests_passed == tests_run) ? 0 : 1;
}
