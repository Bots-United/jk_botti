//
// JK_Botti - be more human!
//
// bot_query_hook.cpp
//

#ifndef _WIN32
#include <string.h>
#endif
#include "asm_string.h"

#include <malloc.h>
#include <stdlib.h>
#include <memory.h>

#include "bot_query_hook.h"

//
static bool msg_get_string(const unsigned char * &msg, size_t &len, char * name, int nlen)
{
	int i = 0;
	
	while(*msg != 0)
	{
		if(i < nlen)
			name[i++]=*msg;
		
		msg++;
		if(--len == 0)
			return(false);
	}
	
	if(i < nlen)
		name[i]=0;
	
	msg++;
	if(--len == 0)
		return(false);
	
	return(true);
}

// find bot connection times and replace
ssize_t PASCAL handle_player_reply(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
/*
(int32) Header
(char) Character 'D'
--
(byte) Player Count
(byte) Player Number
(string) Player Name
(int32) Player Score
(float32) Player Time
(byte) Player Number
(string) Player Name
(int32) Player Score
(float32) Player Time
*/
	unsigned char * newmsg = (unsigned char *)alloca(length);
	memcpy(newmsg, message, length);
	
	size_t len = length - 5;
	const unsigned char * msg = (const unsigned char*)message + 5;
	
	int i, pcount;
	size_t offset;
	char pname[64];
	
	//get player count
	pcount = *msg++;
	if(--len == 0)
		return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
	
	//parse player slots
	for(i = 0; i < pcount; i++) 
	{		
		// skip player number
		msg++;
		if(--len == 0)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
		
		memset(pname, 0, sizeof(pname));
		
		// detect bot by name
		if(!msg_get_string(msg, len, pname, sizeof(pname)))
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
		
		// skip score
		if(len <= 4) 
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
		msg+=4;
		len-=4;
		
		// check that there is enough bytes left
		if(len < 4) 
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len)); 
		
		offset = (size_t)msg - (size_t)message;
		
		BotReplaceConnectionTime(pname, (float*)&newmsg[offset]);
		
		msg+=4;
		len-=4;
		
		if(len == 0)
			return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
	}
	
	return(call_original_sendto(socket, newmsg, length, flags, dest_addr, dest_len));
}

//
ssize_t PASCAL sendto_hook(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	const unsigned char * orig_buf = (unsigned char*)message;
	
	if(length > 5 && orig_buf[0] == 0xff && orig_buf[1] == 0xff && orig_buf[2] == 0xff && orig_buf[3] == 0xff)
	{
		// check if this is player reply packet (ÿÿÿÿD)
		if(orig_buf[4] == 'D' && bot_conntimes >= 1)
		{
			return(handle_player_reply(socket, message, length, flags, dest_addr, dest_len));
		}
	}
	
	return(call_original_sendto(socket, message, length, flags, dest_addr, dest_len));
}
