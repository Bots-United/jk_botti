// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// support_meta.h - generic support macros

/*
 * Copyright (c) 2001 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef SUPPORT_META_H
#define SUPPORT_META_H

#include <string.h>		// strcpy(), strncat()

void do_exit(int exitval);

// Unlike snprintf(), strncpy() doesn't necessarily null-terminate the
// target.  It appears the former function reasonably considers the given
// size to be "max size of target string" (including the null-terminator),
// whereas strncpy() strangely considers the given size to be "total number
// of bytes to copy".  Note strncpy() _always_ writes n bytes, whereas
// snprintf() writes a _max_ of n bytes (incl the NULL).  If strncpy()
// needs to write extra bytes to reach n, it uses NULLs, so the target
// _can_ be null-terminated, but only if the source string would have fit
// anyway -  in which case why not just use strcpy() instead?
//
// Thus, it appears strncpy() is not only unsafe, it's also inefficient,
// and seemingly no better than plain strcpy() anyway.
//
// With this logic, strncpy() doesn't appear to be much of a "str" function
// at all, IMHO.
//
// Strncat works better, although it considers the given size to be "number
// of bytes to append", and doesn't include the null-terminator in that
// count.  Thus, we can use it for what we want to do, by setting the
// target to zero-length (NULL in first byte), and copying n-1 bytes
// (leaving room for the null-termiator).
//
// Why does it have to be soo haaard...

// Also note, some kind of wrapper is necessary to group the two
// statements into one, for use in situations like non-braced else
// statements.

// Technique 1: use "do..while":
#if 0
#define STRNCPY(dst, src, size) \
	do { strcpy(dst, "\0"); strncat(dst, src, size-1); } while(0)
#endif

// Technique 2: use parens and commas:
#if 0
#define STRNCPY(dst, src, size) \
	(strcpy(dst, "\0"), strncat(dst, src, size-1))
#endif

// Technique 3: use inline
inline char *STRNCPY(char *dst, const char *src, int size) {
	strcpy(dst, "\0");
	return(strncat(dst, src, size-1));
}

// Renamed string functions to be clearer.
inline int strmatch(const char *s1, const char *s2) {
	if(!s1 || !s2) 
		return(0);
	else 
		return(!jkstrcmp(s1, s2));
}
inline int strnmatch(const char *s1, const char *s2, size_t n) {
	if(!s1 || !s2) 
		return(0);
	else 
		return(!jkstrncmp(s1, s2, n));
}
inline int strcasematch(const char *s1, const char *s2) {
	if(!s1 || !s2) 
		return(0);
	else 
		return(!strcasecmp(s1, s2));
}
inline int strncasematch(const char *s1, const char *s2, size_t n) {
	if(!s1 || !s2) 
		return(0);
	else 
		return(!strncasecmp(s1, s2, n));
}

// Turn a variable/function name into the corresponding string, optionally
// stripping off the leading "len" characters.  Useful for things like
// turning 'pfnClientCommand' into "ClientCommand" so we don't have to
// specify strings used for all the debugging/log messages.
#define STRINGIZE(name, len)		#name+len


// Max description length for metamod.ini and other places.
#define MAX_DESC_LEN 256


// For various character string buffers.
#define MAX_STRBUF_LEN 1024


// Our own boolean type, for stricter type matching.
typedef enum {
	mFALSE = 0,
	mTRUE,
} mBOOL;

// Like C's errno, for our various functions; describes causes of failure
// or mFALSE returns.
typedef enum {
	ME_NOERROR = 0,
	ME_FORMAT,			// invalid format
	ME_COMMENT,			// ignored comment
	ME_ALREADY,			// request had already been done
	ME_DELAYED,			// request is delayed
	ME_NOTALLOWED,		// request not allowed
	ME_SKIPPED,			// request is being skipped for whatever reason
	ME_BADREQ,			// invalid request for this <whatever>
	ME_ARGUMENT,		// invalid arguments
	ME_NULLRESULT,		// resulting data was empty or null
	ME_MAXREACHED,		// reached max/limit
	ME_NOTUNIQ,			// not unique (ambigious match)
	ME_NOTFOUND,		// in find operation, match not found
	ME_NOFILE,			// file empty or missing
	ME_NOMEM,			// malloc failed
	ME_BADMEMPTR,		// invalid memory address
	ME_OSNOTSUP,		// OS doesn't support this operation
	ME_DLOPEN,			// failed to open shared lib/dll
	ME_DLMISSING,		// symbol missing in lib/dll
	ME_DLERROR,			// some other error encountered calling functions from dll
	ME_IFVERSION,		// incompatible interface version
} META_ERRNO;
extern META_ERRNO meta_errno;

#define RETURN_ERRNO(retval, errval) \
	do { meta_errno=errval; return(retval); } while(0)

#define RETURN_LOGERR_ERRNO(errargs, retval, errval) \
	do { META_ERROR errargs ; meta_errno=errval; return(retval); } while(0)

#endif /* SUPPORT_META_H */
