#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_FILENAME_SIZE 24
#define MAX_PASSWORD_SIZE 9

// Communication Functions
int createUDPSocket();
int createTCPSocket();
void sendUDPMessage(const char* message, char* reply, char* ASPort, char* ASIP);
void sendTCPMessage(const char* message, char* reply, char* ASPort, char* ASIP);

#endif // COMMUNICATION_H
