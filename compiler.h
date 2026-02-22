#ifndef COMPILER_H
#define COMPILER_H

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
   #define likely(x) (x)
   #define unlikely(x) (x)
#else
   #define likely(x) __builtin_expect((long int)!!(x), true)
   #define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif

// memccpy is available on POSIX (Linux glibc, mingw)
#if defined(__linux__) || defined(_WIN32)
   #define HAVE_MEMCCPY 1
#endif

#endif // COMPILER_H
