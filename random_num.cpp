// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
	#define likely(x) (x)
	#define unlikely(x) (x)
#else
	#define likely(x) __builtin_expect((long int)!!(x), true)
	#define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif

static unsigned int rnd_idnum[2] = {1, 1};

/* generates a random 32bit integer */
static unsigned int fast_generate_random(void)
{
   rnd_idnum[0] ^= rnd_idnum[1] << 5;
   
   rnd_idnum[0] *= 1664525L;
   rnd_idnum[0] += 1013904223L;
   
   rnd_idnum[1] *= 1664525L;
   rnd_idnum[1] += 1013904223L;
   
   rnd_idnum[1] ^= rnd_idnum[0] << 3;
   
   return rnd_idnum[0];
}


void fast_random_seed(unsigned int seed)
{
   rnd_idnum[0] = seed;
   rnd_idnum[1] = ~(seed + 6);
   rnd_idnum[1] = fast_generate_random();
}


/* supports range INT_MIN, INT_MAX */
int RANDOM_LONG2(int lLow, int lHigh) 
{
   const double c_divider = ((unsigned long long)1) << 32; // div by (1<<32)
   double rnd;
   
   if(unlikely(lLow >= lHigh))
      return(lLow);
   
   rnd = fast_generate_random();
   rnd *= (double)lHigh - (double)lLow + 1.0;
   rnd /= c_divider; // div by (1<<32)
   
   return (int)(rnd + (double)lLow);
}


float RANDOM_FLOAT2(float flLow, float flHigh) 
{
   const double c_divider = (((unsigned long long)1) << 32) - 1; // div by (1<<32)-1
   double rnd;
   
   if(unlikely(flLow >= flHigh))
      return(flLow);
   
   rnd = fast_generate_random();
   rnd *= (double)flHigh - (double)flLow;
   rnd /= c_divider; // div by (1<<32)-1
   
   return (float)(rnd + (double)flLow);
}
