#ifndef SAFE_SNPRINTF_H
#define SAFE_SNPRINTF_H

extern char * safe_strcopy(char * dst, size_t dst_size, const char *src);

extern void safevoid_snprintf(char* s, size_t n, const char* format, ...);
extern void safevoid_vsnprintf(char* s, size_t n, const char *format, va_list ap);
extern int safe_snprintf(char* s, size_t n, const char* format, ...);
extern int safe_vsnprintf(char* s, size_t n, const char *format, va_list src_ap);

#endif
