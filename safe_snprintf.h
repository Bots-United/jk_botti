#ifndef SAFE_SNPRINTF_H
#define SAFE_SNPRINTF_H

extern char * safe_strcopy(char * dst, size_t dst_size, const char *src);

extern void safevoid_snprintf(char* s, size_t n, const char* format, ...);
extern void safevoid_vsnprintf(char* s, size_t n, const char *format, va_list ap);

#endif
