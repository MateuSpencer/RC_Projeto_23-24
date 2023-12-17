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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_UDP_REQUEST_BUFFER_SIZE 21 //LIN / LOU
#define MIN_UDP_REQUEST_BUFFER_SIZE 4 //LST
#define MAX_UDP_REPLY_BUFFER_SIZE 6000

#define MAX_TCP_REQUEST_BUFFER_SIZE 6000
#define MAX_TCP_REPLY_BUFFER_SIZE 6000

#define UID_SIZE 6
#define PASSWORD_SIZE 8
#define MAX_BID_SIZE 6
#define MAX_AID_SIZE 3
#define MAX_FILENAME_SIZE 24
#define MAX_FSIZE_LEN 8
#define MAX_FSIZE_NUM 0xA00000 // 10 MB
#define TIMEOUT 5
#define MAX_TCP_CONNECTIONS 5


// Communication Functions
int createUDPSocket();
int createTCPSocket();

#endif // COMMON_H
