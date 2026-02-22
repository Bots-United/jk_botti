//
// JK_Botti - be more human!
//
// safe_snprintf.cpp
//

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "safe_snprintf.h"

#include "compiler.h"

// Acts very much like safevoid_snprintf(dst, dst_size, "%s", src)
//    if src is null "(null)" is written
//    returns dst
char* safe_strcopy(char* dst, size_t dst_size, const char *src)
{
   if (unlikely(!dst_size))
      return dst;

   if (unlikely(!src))
      src = "(null)";

#ifdef HAVE_MEMCCPY
   // memccpy copies until '\0' found or dst_size-1 bytes copied,
   // returns NULL if '\0' was not found within the limit
   if (!memccpy(dst, src, '\0', dst_size - 1))
      dst[dst_size - 1] = '\0';
#else
   {
      size_t i;

      for (i = 0; likely(i < dst_size - 1) && likely(src[i] != '\0'); i++)
         dst[i] = src[i];

      dst[i] = '\0';
   }
#endif

   return dst;
}

void safevoid_vsnprintf(char* s, size_t n, const char *format, va_list ap)
{
   int res;

   if (unlikely(!s) || unlikely(!n))
      return;

   // If the format string is empty, nothing to do.
   if (unlikely(!format) || unlikely(!*format))
   {
      s[0] = 0;
      return;
   }

   res = vsnprintf(s, n, format, ap);

   // w32api returns -1 on too long write, glibc returns number of bytes it could have written if there were enough space
   // w32api doesn't write null at all, some buggy glibc don't either
   if (unlikely(res < 0) || unlikely((size_t)res >= n))
      s[n-1] = 0;
}

void safevoid_snprintf(char* s, size_t n, const char* format, ...)
{
   va_list ap;

   va_start(ap, format);
   safevoid_vsnprintf(s, n, format, ap);
   va_end(ap);
}
