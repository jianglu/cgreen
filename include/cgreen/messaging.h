#ifndef MESSAGING_HEADER
#define MESSAGING_HEADER

#ifdef __cplusplus
  extern "C" {
#endif

#if defined WINCE || defined WIN32
#include <stdio.h>
#else
#include <sys/msg.h>
#endif

int start_cgreen_messaging(int tag);
void send_cgreen_message(int messaging, int result);
int receive_cgreen_message(int messaging);

#ifdef __cplusplus
    }
#endif

#endif
