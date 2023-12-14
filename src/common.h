#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER_SIZE 4000
#define UID_SIZE 6
#define PASSWORD_SIZE 8
#define MAX_FILENAME_SIZE 24
#define MAX_FSIZE_LEN 8
#define MAX_FSIZE_NUM 0xA00000 // 10 MB

/*
// Communication Functions
int createUDPSocket();
int createTCPSocket();
*/
#endif // COMMON_H
