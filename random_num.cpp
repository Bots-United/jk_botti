#include "compiler.h"

static unsigned int rnd_idnum[2] = {1, 1};

#define ROL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

/*
 * Generates a random 32-bit integer using two cross-coupled LCGs.
 *
 * Uses different LCG constants for each state half so the sequences
 * are fundamentally independent:
 *   state[0]: Numerical Recipes (Knuth) — mult 1664525, inc 1013904223
 *   state[1]: Borland Delphi            — mult 22695477, inc 1
 *
 * Cross-coupling via rotate-and-XOR mixes bits between the two halves.
 * Rotation amounts (16, 5) and LCG pairing were selected by brute-force
 * optimization over all combinations, scored on chi-squared uniformity,
 * bit balance, serial correlation, avalanche effect, and gap tests.
 */
static unsigned int fast_generate_random(void)
{
   rnd_idnum[0] ^= ROL32(rnd_idnum[1], 16);

   rnd_idnum[0] *= 1664525L;
   rnd_idnum[0] += 1013904223L;

   rnd_idnum[1] *= 22695477L;
   rnd_idnum[1] += 1L;

   rnd_idnum[1] ^= ROL32(rnd_idnum[0], 5);

   return rnd_idnum[0];
}

void fast_random_seed(unsigned int seed)
{
   int i;

   rnd_idnum[0] = seed;
   rnd_idnum[1] = ~(seed + 6);

   // warmup: run several rounds to diffuse seed across both state halves
   for (i = 0; i < 4; i++)
      fast_generate_random();
}

/* returns random integer in range [lLow, lHigh] inclusive */
int RANDOM_LONG2(int lLow, int lHigh)
{
   const double c_divider = ((unsigned long long)1) << 32; // div by (1<<32)
   double rnd;
   int result;

   if (unlikely(lLow >= lHigh))
      return(lLow);

   rnd = fast_generate_random();
   rnd *= (double)lHigh - (double)lLow + 1.0;
   rnd /= c_divider; // div by (1<<32)

   result = (int)(rnd + (double)lLow);

   // clamp: floating point rounding can produce lHigh+1 at the edge
   if (unlikely(result > lHigh))
      result = lHigh;

   return result;
}

/* returns random float in range [flLow, flHigh] inclusive */
float RANDOM_FLOAT2(float flLow, float flHigh)
{
   const double c_divider = (((unsigned long long)1) << 32) - 1; // div by (1<<32)-1
   double rnd;

   if (unlikely(flLow >= flHigh))
      return(flLow);

   rnd = fast_generate_random();
   rnd *= (double)flHigh - (double)flLow;
   rnd /= c_divider; // div by (1<<32)-1

   return (float)(rnd + (double)flLow);
}
