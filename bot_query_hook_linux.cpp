//
// JK_Botti - be more human!
//
// bot_query_hook_linux.cpp
//

#ifndef _WIN32

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <sys/mman.h>
#include <pthread.h>

#include <memory.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "bot_query_hook.h"

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr) (((addr)+PAGE_SIZE-1)&PAGE_MASK)

//
// Linux work, based on my "Linux code for dynamic linkents" from metamod-p
//

// jmp rel32 forwarder (5 bytes)
#define JMP_INSN_SIZE 5
#define BYTES_SIZE JMP_INSN_SIZE

#define ENDBR32_SIZE 4

typedef ssize_t (*sendto_func)(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

static bool is_sendto_hook_setup = false;

//pointer to original sendto
static sendto_func sendto_original;

//contains jmp to replacement_sendto @sendto_original
static unsigned char sendto_new_bytes[BYTES_SIZE];

//contains original bytes of sendto
static unsigned char sendto_old_bytes[BYTES_SIZE];

//Mutex for our protection
static pthread_mutex_t mutex_replacement_sendto = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//constructs jmp forwarder
static void construct_jmp_instruction(void *x, void *place, void *target)
{
   unsigned char *p = (unsigned char *)x;
   // jmp rel32 (offset relative to end of jmp instruction)
   p[0] = 0xe9;
   unsigned long offset = ((unsigned long)target) - (((unsigned long)place) + BYTES_SIZE);
   memcpy(p + 1, &offset, sizeof(unsigned long));
}

// endbr32 landing pad (f3 0f 1e fb): emitted at function entry under CET/IBT.
static bool has_endbr32(const void *p)
{
   static const unsigned char endbr32[ENDBR32_SIZE] = { 0xf3, 0x0f, 0x1e, 0xfb };
   return memcmp(p, endbr32, ENDBR32_SIZE) == 0;
}

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
   // in PIC/PIE builds where EBX is the GOT pointer.
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
   atomic_patch_sendto((void*)sendto_original, sendto_old_bytes, BYTES_SIZE);
}

//resets new sendto
inline void reset_sendto_hook(void)
{
   //Copy new sendto bytes back
   atomic_patch_sendto((void*)sendto_original, sendto_new_bytes, BYTES_SIZE);
}

// Replacement sendto function
static ssize_t FORCE_STACK_ALIGN __replacement_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
   return sendto_hook(socket, message, length, flags, dest_addr, dest_len);
}

//
ssize_t call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
   /* Emulate sendto using sendmsg.. this is faster than restoring/replacing sendto() code. */
   /* Also, internally libc sendto() is wrapper around sendmsg(). */
   struct iovec iov = {0,};
   iov.iov_base = (void*)message;
   iov.iov_len = length;
   struct msghdr msg = {0,};
   msg.msg_name = (void*)dest_addr;
   msg.msg_namelen = dest_len;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;

   return sendmsg(socket, &msg, flags);
}

//
bool hook_sendto_function(void)
{
   if(is_sendto_hook_setup)
      return(true);

   is_sendto_hook_setup = false;

   //metamod-p p31 parses elf structures, we find function easier&better way:
   void * sym_ptr = (void*)&sendto;
   while(*(unsigned short*)sym_ptr == 0x25ff) {
      sym_ptr = **(void***)((char *)sym_ptr + 2);
   }

   // Patch past the endbr32 landing pad (if present) so the PLT's indirect
   // branch still lands on a valid endbr32 under CET/IBT.
   if(has_endbr32(sym_ptr))
      sym_ptr = (char *)sym_ptr + ENDBR32_SIZE;

   sendto_original = (sendto_func)sym_ptr;

   //Backup old bytes of "sendto" function
   memcpy(sendto_old_bytes, (void*)sendto_original, BYTES_SIZE);

   //Construct new bytes: "jmp offset[replacement_sendto] @ sendto_original"
   construct_jmp_instruction((void*)&sendto_new_bytes[0], (void*)sendto_original, (void*)&__replacement_sendto);

   //Check if bytes overlap page border. The atomic publish writes an 8-byte
   //window, so make the whole window writable (not just BYTES_SIZE).
   unsigned long start_of_page = PAGE_ALIGN((long)sendto_original) - PAGE_SIZE;
   unsigned long size_of_pages = 0;

   if((unsigned long)sendto_original + sizeof(unsigned long long) > PAGE_ALIGN((unsigned long)sendto_original))
   {
      //bytes are located on two pages
      size_of_pages = PAGE_SIZE*2;
   }
   else
   {
      //bytes are located entirely on one page.
      size_of_pages = PAGE_SIZE;
   }

   //Remove PROT_READ restriction
   if(mprotect((void*)start_of_page, size_of_pages, PROT_READ|PROT_WRITE|PROT_EXEC))
   {
      UTIL_ConsolePrintf("Couldn't initialize sendto hook, mprotect failed: %i.  Exiting...\n", errno);
      return(false);
   }

   //Write our own jmp-forwarder on "sendto"
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
   pthread_mutex_lock(&mutex_replacement_sendto);

   //reset sendto hook
   restore_original_sendto();

   //unlock
   pthread_mutex_unlock(&mutex_replacement_sendto);

   is_sendto_hook_setup = false;

   return(true);
}

#endif /*!_WIN32*/
