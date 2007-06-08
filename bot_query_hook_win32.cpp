#ifdef _WIN32

#include <memory.h>
#include <string.h>
#include <stdio.h>

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
	((unsigned char *)(x))[0] = 0xe9; \
	*(unsigned long *)((char *)(x) + 1) = ((unsigned long)(target)) - (((unsigned long)(place)) + 5); \
}
	
//opcode + sizeof pointer
#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)

typedef ssize_t (PASCAL * sendto_func)(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

static bool is_sendto_hook_setup = false;

//pointer to original sendto
static sendto_func sendto_original;

//contains jmp to replacement_sendto @sendto_original
static unsigned char sendto_new_bytes[BYTES_SIZE];

//contains original bytes of sendto
static unsigned char sendto_old_bytes[BYTES_SIZE];

//Mutex for our protection
static CRITICAL_SECTION mutex_replacement_sendto;

//restores old sendto
inline void restore_original_sendto(void)
{
	//Copy old sendto bytes back
	memcpy((void*)sendto_original, sendto_old_bytes, BYTES_SIZE);
}

//resets new sendto
inline void reset_sendto_hook(void)
{
	//Copy new sendto bytes back
	memcpy((void*)sendto_original, sendto_new_bytes, BYTES_SIZE);
}

// Replacement sendto function
static ssize_t __replacement_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	static int is_original_restored = 0;
	int was_original_restored = is_original_restored;
	
	//Lock before modifing original sendto
	EnterCriticalSection(&mutex_replacement_sendto);
	
	//restore old sendto
	if(!is_original_restored)
	{
		restore_original_sendto();
		
		is_original_restored = 1;
	}
	
	ssize_t ret = sendto_hook(socket, message, length, flags, dest_addr, dest_len);
	
	//reset hook
	if(!was_original_restored)
	{
		//reset sendto hook
		reset_sendto_hook();
		
		is_original_restored = 0;
	}
	
	//unlock
	LeaveCriticalSection(&mutex_replacement_sendto);
	
	return(ret);
}

//
ssize_t call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	return (*sendto_original)(socket, message, length, flags, dest_addr, dest_len);
}

//
bool hook_sendto_function(void)
{
	DWORD tmp = 0;
	
	if(is_sendto_hook_setup)
		return(true);
	
	InitializeCriticalSection(&mutex_replacement_sendto);
	
	is_sendto_hook_setup = false;
	
	sendto_original = (sendto_func)GetProcAddress(GetModuleHandle("wsock32.dll"), "sendto");
	
	//Backup old bytes of "sendto" function
	memcpy(sendto_old_bytes, (void*)sendto_original, BYTES_SIZE);
	
	//Construct new bytes: "jmp offset[replacement_sendto] @ sendto_original"
	construct_jmp_instruction((void*)&sendto_new_bytes[0], (void*)sendto_original, (void*)&__replacement_sendto);
	
	//Remove readonly restriction
	if(!VirtualProtect((void*)sendto_original, BYTES_SIZE, PAGE_READWRITE, &tmp))
	{
		UTIL_ConsolePrintf("Couldn't initialize sendto hook, VirtualProtect failed: %i.  Exiting...\n", GetLastError());
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
	EnterCriticalSection(&mutex_replacement_sendto);
	
	//reset sendto hook
	restore_original_sendto();
	
	//unlock
	LeaveCriticalSection(&mutex_replacement_sendto);
	
	DeleteCriticalSection(&mutex_replacement_sendto);
	
	is_sendto_hook_setup = false;
	
	return(true);
}

/*
#include "bot_query_hook.h"

static bool is_active = false;

//Changes import function for hooking and stores original pointer
unsigned long ExchangeImport(HMODULE module, unsigned long funcAddress, unsigned long newAddress, unsigned long * oldAddress)
{
	union { 
		unsigned long mem;
		IMAGE_DOS_HEADER        * dos;
		IMAGE_NT_HEADERS        * pe;
		IMAGE_IMPORT_DESCRIPTOR * import;
	} mem;
	union {
		unsigned long mem;
		IMAGE_THUNK_DATA *thunk;
	} mem2;
	
	if(!module || !newAddress || !funcAddress)
		return(0);
	
	//Check if valid dos header
	mem.mem = (unsigned long)module;
	if(IsBadReadPtr(mem.dos, sizeof(*mem.dos)) || mem.dos->e_magic != IMAGE_DOS_SIGNATURE)
		return(0);
	
	//Get and check pe header
	mem.mem = (unsigned long)module + (unsigned long)mem.dos->e_lfanew;
	if(IsBadReadPtr(mem.pe, sizeof(*mem.pe)) || mem.pe->Signature != IMAGE_NT_SIGNATURE)
		return(0);
	
	//Get imports
	mem.mem = (unsigned long)module + (unsigned long)mem.pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	if(mem.mem == (unsigned long)module)
		return(0);
	
	while(!IsBadReadPtr(mem.import, sizeof(*mem.import)) && mem.import->Name)
	{
		mem2.mem = (unsigned long)module + (unsigned long)mem.import->FirstThunk;
		while(!IsBadReadPtr(mem2.thunk, sizeof(*mem2.thunk)) && mem2.thunk->u1.Function)
		{
			if((unsigned long)mem2.thunk->u1.Function == (unsigned long)funcAddress)
			{
				if(oldAddress)
					*oldAddress = (unsigned long)mem2.thunk->u1.Function;
				
				VirtualProtect(&mem2.thunk->u1.Function, 4, PAGE_READWRITE, &mem.mem);
				if(!IsBadWritePtr(&mem2.thunk->u1.Function, sizeof(unsigned long)))
					*(unsigned long*)&mem2.thunk->u1.Function = (unsigned long)newAddress;
				VirtualProtect(&mem2.thunk->u1.Function, 4 , mem.mem, &mem.mem);
				
				return(1);
			}
				
			mem2.thunk++;
		}
		
		mem.import++;
	}
	
	return(0);
}

//
ssize_t call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	return sendto(socket, (const char*)message, length, flags, dest_addr, dest_len);
}

//
bool hook_sendto_function(void)
{
	if(is_active)
		return(true);
	
	is_active = false;
	
	// search by function address because wsock32 imports in swds are done by ordinals, not names.
	if(!ExchangeImport(
		GetModuleHandle("swds.dll"),
		(unsigned long)GetProcAddress(GetModuleHandle("wsock32.dll"), "sendto"), 
		(unsigned long)&sendto_hook, 0))
	{
		UTIL_ConsolePrintf("ExchangeImport(\"swds.dll\",\"wsock32.dll\":\"sendto\",0x%x,0x%x) == 0", (unsigned long)&sendto_hook, 0);
		return(false);
	}
	
	is_active = true;
	
	return(true);
}

//
bool unhook_sendto_function(void)
{
	if(!is_active)
		return(true);
	
	ExchangeImport(
		GetModuleHandle("swds.dll"),
		(unsigned long)&sendto_hook, 
		(unsigned long)GetProcAddress(GetModuleHandle("wsock32.dll"), "sendto"), 0
	);
	
	is_active = false;
	
	return(true);
}*/

#endif /*_WIN32*/
