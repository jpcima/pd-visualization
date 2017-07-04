#pragma once
#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

int socketpair(int family, int type, int protocol, SOCKET sv[2]);

#ifdef __cplusplus
}  // extern "C"
#endif
