//
// JK_Botti - be more human!
//
// safe_snprintf.cpp
//

#ifndef _WIN32
#include <string.h>
#endif
#include "asm_string.h"

#include <stdarg.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <limits.h>

#include "safe_snprintf.h"

// Microsoft's msvcrt.dll:vsnprintf is buggy and so is vsnprintf on some glibc versions.
// We use wrapper function to fix bugs.
//  from: http://sourceforge.net/tracker/index.php?func=detail&aid=1083721&group_id=2435&atid=102435
int safe_vsnprintf(char* s, size_t n, const char *format, va_list src_ap) 
{ 
	va_list ap;
	int res;
	char *tmpbuf;
	size_t bufsize = n;
	
	if(s && n>0)
		s[0]=0;
	
	// If the format string is empty, nothing to do.
	if(!format || !*format)
		return(0);
	
	// The supplied count may be big enough. Try to use the library
     	// vsnprintf, fixing up the case where the library function
	// neglects to terminate with '/0'.
	if(n > 0)
	{
		// A NULL destination will cause a segfault with vsnprintf.
		// if n > 0.  Nor do we want to copy our tmpbuf to NULL later.
		if(!s)
			return(-1);
		
		va_copy(ap, src_ap);
		res = vsnprintf(s, n, format, ap);
		va_end(ap);
		
		if(res > 0) {
			if((unsigned)res == n)
				s[res - 1] = 0;
	  		return(res);
		}
		
		// If n is already larger than INT_MAX, increasing it won't
		// help.
		if(n > INT_MAX)
			return(-1);
		
		// Try a larger buffer.
		bufsize *= 2;
	}
	
	if(bufsize < 1024)
		bufsize = 1024;
	
	tmpbuf = (char *)malloc(bufsize * sizeof(char));
	if(!tmpbuf)
		return(-1);
	
	va_copy(ap, src_ap);
  	res = vsnprintf(tmpbuf, bufsize, format, ap);
  	va_end(ap);
	
	// The test for bufsize limit is probably not necesary
	// with 2GB address space limit, since, in practice, malloc will
	// fail well before INT_MAX.
	while(res < 0 && bufsize <= INT_MAX) 
	{
		char * newbuf;
		
		bufsize *= 2;
		newbuf = (char*)realloc(tmpbuf, bufsize * sizeof(char));
		
		if(!newbuf)
			break;
		
		tmpbuf = newbuf;
		
		va_copy(ap, src_ap);
		res = vsnprintf(tmpbuf, bufsize, format, ap);
		va_end(ap);
	}
	
	if(res > 0 && n > 0) 
	{
		if(n > (unsigned)res)
			memcpy(s, tmpbuf, (res + 1) * sizeof (char));
		else {
			memcpy(s, tmpbuf, (n - 1) * sizeof (char));
			s[n - 1] = 0;
		}
	}
	
	free(tmpbuf);
	return(res);
}

int safe_snprintf(char* s, size_t n, const char* format, ...) 
{
	int res;
	va_list ap;
	
	va_start(ap, format);
	res = safe_vsnprintf(s, n, format, ap);
	va_end(ap);
	
	return(res);
}

void safevoid_vsnprintf(char* s, size_t n, const char *format, va_list ap) 
{ 
	int res;
	
	if(!s || n <= 0)
		return;
	
	// If the format string is empty, nothing to do.
	if(!format || !*format) 
	{
		s[0]=0;
		return;
	}
	else if(format[0] == '%' && format[1] == 's' && format[2] == '\0')
	{
		//special case for handling "%s" fast!
		const char *str = va_arg(ap, const char *);
		size_t i;
		
		if(!str)
			str = "(null)";
		
		i = 0;
		while(str[i] != '\0' && i < n)
		{
			s[i] = str[i];
			i++;
		}
		
		if(i < n)
			s[i] = '\0';
		else if(i == n)
			s[i-1] = '\0';
		
		return;
	}
	
	res = vsnprintf(s, n, format, ap);
	
	// w32api returns -1 on too long write, glibc returns number of bytes it could have written if there were enough space
	// w32api doesn't write null at all, some buggy glibc don't either
	if(res < 0 || (size_t)res >= n)
		s[n-1]=0;
}

void safevoid_snprintf(char* s, size_t n, const char* format, ...) 
{
	va_list ap;
	
	va_start(ap, format);
	safevoid_vsnprintf(s, n, format, ap);
	va_end(ap);
}
