#include "communication.h"

int createUDPSocket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }
    return udpSocket;
}

int createTCPSocket() {
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }
    return tcpSocket;
}