// Mock winsock2.h for testing bot_query_hook_win32.cpp on Linux
#ifndef MOCK_WINSOCK2_H
#define MOCK_WINSOCK2_H

#ifndef _MOCK_DWORD_DEFINED
#define _MOCK_DWORD_DEFINED
typedef unsigned long DWORD;
#endif

#define SOCKET_ERROR (-1)

struct sockaddr;

typedef struct { unsigned long len; char *buf; } WSABUF;

int WSASendTo(int s, WSABUF *lpBuffers, DWORD dwBufferCount,
              DWORD *lpNumberOfBytesSent, DWORD dwFlags,
              const struct sockaddr *lpTo, int iToLen,
              void *lpOverlapped, void *lpCompletionRoutine);
int WSAGetLastError(void);

#endif // MOCK_WINSOCK2_H
