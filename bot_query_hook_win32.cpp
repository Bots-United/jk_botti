//
// JK_Botti - be more human!
//
// bot_query_hook_win32.cpp
//

#ifdef _WIN32

#include <string.h>

#include <memory.h>
#include <stdio.h>

#include <winsock2.h>
#include <windows.h>

#include "bot_query_hook.h"

//
// win32, based on my "Linux code for dynamic linkents" from metamod-p
//

// 0xe9 is opcode for our function forwarder
#define JMP_SIZE 1

//pointer size on x86-32: 4 bytes
#define PTR_SIZE sizeof(void*)

//constructs new jmp forwarder
#define construct_jmp_instruction(x, place, target) { \
   unsigned long _jmp_offset = ((unsigned long)(target)) - (((unsigned long)(place)) + 5); \
   ((unsigned char *)(x))[0] = 0xe9; \
   memcpy((char *)(x) + 1, &_jmp_offset, sizeof(unsigned long)); \
}

//opcode + sizeof pointer
#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)

typedef ssize_t (PASCAL * sendto_func)(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

static bool is_sendto_hook_setup = false;

// One patched sendto per DLL. The engine may reach sendto through ws2_32.dll
// or wsock32.dll depending on engine build, so we patch each that is loaded.
struct sendto_hook_t
{
   sendto_func original;                 //patched sendto
   unsigned char new_bytes[BYTES_SIZE];  //jmp to __replacement_sendto
   unsigned char old_bytes[BYTES_SIZE];  //saved original bytes
};

static sendto_hook_t sendto_hooks[2];
static int num_sendto_hooks = 0;

//Mutex for our protection
static CRITICAL_SECTION mutex_replacement_sendto;

// Atomically publish `patch` (<= 8 bytes) over the 8-byte window at the sendto
// entry, preserving the untouched trailing bytes, so a concurrent caller never
// observes a half-written jmp. The entry is 16-byte aligned in practice, so the
// locked 8-byte store stays within one cache line.
static inline void atomic_patch_sendto(void *dst, const unsigned char *patch, size_t patchlen)
{
   unsigned long long want;

   static_assert(BYTES_SIZE <= sizeof(want));

   memcpy(&want, dst, sizeof(want));   // current 8 bytes
   memcpy(&want, patch, patchlen);     // splice in the patch, keep the rest

   unsigned int nlo = (unsigned int)want;
   unsigned int nhi = (unsigned int)(want >> 32);

   // lock cmpxchg8b store. EBX is saved/restored via xchg so this stays valid
   // in PIC/PIE builds where EBX is the GOT pointer (note: needed as this file
   // is unit-tested on linux target).
   __asm__ __volatile__ (
      "movl    (%%esi), %%eax\n\t"     // expected low  = *dst
      "movl    4(%%esi), %%edx\n\t"    // expected high = *dst
      "xchgl   %%ebx, %0\n\t"          // EBX <- new low (stash caller's EBX)
      "1:\n\t"
      "lock cmpxchg8b (%%esi)\n\t"
      "jnz     1b\n\t"
      "xchgl   %%ebx, %0\n\t"          // restore EBX
      : "+r"(nlo)
      : "S"(dst), "c"(nhi)
      : "eax", "edx", "cc", "memory");
}

//restores old sendto
inline void restore_original_sendto(void)
{
   //Copy old sendto bytes back
   for(int i = 0; i < num_sendto_hooks; i++)
      atomic_patch_sendto((void*)sendto_hooks[i].original, sendto_hooks[i].old_bytes, BYTES_SIZE);
}

//resets new sendto
inline void reset_sendto_hook(void)
{
   //Copy new sendto bytes back
   for(int i = 0; i < num_sendto_hooks; i++)
      atomic_patch_sendto((void*)sendto_hooks[i].original, sendto_hooks[i].new_bytes, BYTES_SIZE);
}

// Replacement sendto function
static ssize_t PASCAL FORCE_STACK_ALIGN __replacement_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
   return sendto_hook(socket, message, length, flags, dest_addr, dest_len);
}

//
ssize_t PASCAL call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
   WSABUF iov = {0,};
   iov.buf = (char*)message;
   iov.len = length;
   DWORD num_sent = 0;
   int err;

   err = WSASendTo(socket, &iov, 1, &num_sent, flags, dest_addr, dest_len, NULL, NULL);
   if (err == SOCKET_ERROR) {
      errno = WSAGetLastError();
      return -1;
   }

   return (size_t)num_sent;
}

//
bool hook_sendto_function(void)
{
   DWORD tmp = 0;

   if(is_sendto_hook_setup)
      return(true);

   // Modern engines (e.g. ReHLDS) import sendto by ordinal from ws2_32.dll;
   // legacy engines use wsock32.dll. Patch whichever are loaded. ws2_32 is
   // resolved first; when wsock32's export just forwards to it (same address,
   // as under wine) the duplicate is skipped so we don't patch/restore the
   // same bytes twice.
   static const char * const dll_names[] = { "ws2_32.dll", "wsock32.dll" };

   num_sendto_hooks = 0;

   for(unsigned int i = 0; i < sizeof(dll_names) / sizeof(dll_names[0]); i++)
   {
      HMODULE mod = GetModuleHandle(dll_names[i]);
      if(!mod)
         continue;

      sendto_func addr = (sendto_func)GetProcAddress(mod, "sendto");
      if(!addr)
         continue;

      bool already_hooked = false;
      for(int j = 0; j < num_sendto_hooks; j++)
      {
         if(sendto_hooks[j].original == addr)
         {
            already_hooked = true;
            break;
         }
      }
      if(already_hooked)
         continue;

      sendto_hooks[num_sendto_hooks++].original = addr;
   }

   if(num_sendto_hooks == 0)
   {
      UTIL_ConsolePrintf("Couldn't initialize sendto hook, sendto not found in ws2_32.dll or wsock32.dll.  bot_conntimes disabled.\n");
      return(false);
   }

   InitializeCriticalSection(&mutex_replacement_sendto);

   for(int i = 0; i < num_sendto_hooks; i++)
   {
      //Backup old bytes of "sendto" function
      memcpy(sendto_hooks[i].old_bytes, (void*)sendto_hooks[i].original, BYTES_SIZE);

      //Construct new bytes: "jmp offset[replacement_sendto] @ sendto"
      construct_jmp_instruction((void*)&sendto_hooks[i].new_bytes[0], (void*)sendto_hooks[i].original, (void*)&__replacement_sendto);

      //Remove readonly restriction
      if(!VirtualProtect((void*)sendto_hooks[i].original, BYTES_SIZE, PAGE_READWRITE, &tmp))
      {
         UTIL_ConsolePrintf("Couldn't initialize sendto hook, VirtualProtect failed: %i.  Exiting...\n", GetLastError());
         DeleteCriticalSection(&mutex_replacement_sendto);
         num_sendto_hooks = 0;
         return(false);
      }
   }

   //Write our own jmp-forwarders on "sendto"
   reset_sendto_hook();

   is_sendto_hook_setup = true;

   //done
   return(true);
}

//
bool unhook_sendto_function(void)
{
   if(!is_sendto_hook_setup)
      return(true);

   //Lock before modifing original sendto
   EnterCriticalSection(&mutex_replacement_sendto);

   //reset sendto hook
   restore_original_sendto();

   //unlock
   LeaveCriticalSection(&mutex_replacement_sendto);

   DeleteCriticalSection(&mutex_replacement_sendto);

   num_sendto_hooks = 0;
   is_sendto_hook_setup = false;

   return(true);
}

#endif /*_WIN32*/
