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

#define MAX_UDP_REQUEST_BUFFER_SIZE 21 //LIN / LOU
#define MIN_UDP_REQUEST_BUFFER_SIZE 4 //LST
#define MAX_UDP_REPLY_BUFFER_SIZE 6000 //TODO: what size limit?

#define MAX_TCP_REQUEST_BUFFER_SIZE 4000 //TODO: what size limit? should be enough for the biggest file size allowed and the message - 10 MB+?
#define MAX_TCP_REPLY_BUFFER_SIZE 4000 //TODO: what size limit? should be enough for the biggest file size allowed and the message - 10 MB+?

#define UID_SIZE 6
#define PASSWORD_SIZE 8
#define MAX_BID_SIZE 6
#define MAX_FILENAME_SIZE 24
#define MAX_FSIZE_LEN 8
#define MAX_FSIZE_NUM 0xA00000 // 10 MB
#define TIMEOUT 100000 //TODO: 5 seconds

/*
// Communication Functions
int createUDPSocket();
int createTCPSocket();
*/
#endif // COMMON_H
