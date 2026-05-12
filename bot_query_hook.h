#ifndef BOT_QUERY_HOOK_H
#define BOT_QUERY_HOOK_H

// Duplicated from metamod/osdep.h — bot_query_hook files cannot include osdep.h
// due to its heavy metamod/HL SDK header dependencies.
#ifndef FORCE_STACK_ALIGN
#if defined(__GNUC__) && defined(__i386__)
   #define FORCE_STACK_ALIGN __attribute__((force_align_arg_pointer))
#else
   #define FORCE_STACK_ALIGN
#endif
#endif

#ifdef _WIN32
   #define WIN32_LEAN_AND_MEAN
   #include <windows.h>
   #include <winsock2.h>

   #include <sys/types.h>

   typedef int socklen_t;
#else
   #include <sys/types.h>
   #include <sys/socket.h>

   #define PASCAL
#endif

ssize_t PASCAL sendto_hook(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
ssize_t PASCAL call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
bool hook_sendto_function(void);
bool unhook_sendto_function(void);

void UTIL_ConsolePrintf( const char *fmt, ... );
void BotReplaceConnectionTime(const char * name, float * timeslot);

extern int bot_conntimes;

#endif
