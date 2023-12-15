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

#define MAX_BUFFER_SIZE 4000//TODO: faze out

#define MAX_REQUEST_UDP_BUFFER_SIZE 21 //LIN / LOU
#define MIN_REQUEST_UDP_BUFFER_SIZE 4 //LST
#define MAX_RESPONSE_UDP_BUFFER_SIZE 6000 //TODO: what size limit?

#define MAX_REQUEST_TCP_BUFFER_SIZE 4000 //TODO: what size limit?
#define MIN_REQUEST_TCP_BUFFER_SIZE 4000 ///TODO: what size limit?
#define MAX_RESPONSE_TCP_BUFFER_SIZE 4000 //TODO: what size limit?

#define UID_SIZE 6
#define PASSWORD_SIZE 8
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
