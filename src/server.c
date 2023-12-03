#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>

#include "communication.h"

void handleUDPRequests(char* ASPort);
void handleTCPRequests(char* ASPort);

void createUserDirectory(const char* UID);
void addBid(const char* UID, int AID, int value);
void createAuctionDirectory(int AID, const char* name, const char* asset_fname, int start_value, int timeactive);
void closeAuctionDirectory(int AID);
void unregisterUser(const char* UID);

void handleLoginRequest(char* request, char* response);
void handleOpenAuctionRequest(char* request, char* response);
void handleCloseAuctionRequest(char* request, char* response);
void handleMyAuctionsRequest(char* request, char* response);
void handleMyBidsRequest(char* request, char* response);
void handleListAuctionsRequest(char** response);
void handleShowAssetRequest(char* request, char* response);
void handleBidRequest(char* request, char* response);
void handleShowRecordRequest(char* request, char* response);
void handleLogoutRequest(char* request, char* response);
void handleUnregisterRequest(char* request, char* response);


int main(int argc, char *argv[]) {
    pid_t udpProcess, tcpProcess;
    char ASport[6]; //TODO

    // Create UDP process
    udpProcess = fork();
    if (udpProcess < 0) {
        perror("Error forking UDP process");
        exit(EXIT_FAILURE);
    } else if (udpProcess == 0) {
        handleUDPRequests(ASport);
        exit(EXIT_SUCCESS);
    }

    // Create TCP process
    tcpProcess = fork();
    if (tcpProcess < 0) {
        perror("Error forking TCP process");
        exit(EXIT_FAILURE);
    } else if (tcpProcess == 0) {
        handleTCPRequests(ASport);
        exit(EXIT_SUCCESS);
    }

    // Wait for both child processes to finish
    waitpid(udpProcess, NULL, 0);
    waitpid(tcpProcess, NULL, 0);

    return 0;
}

void handleUDPRequests(char* ASPort) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];
    char* response;

    fd = createUDPSocket();

    // Set up address information for binding
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, ASPort, &hints, &res);
    if (errcode != 0) {
        perror("getaddrinfo failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified address
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);  // No longer needed

    // Loop to receive and process UDP messages
    while (1) {
        addrlen = sizeof(addr);

        // Receive data from the client
        n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        char* action = strtok(buffer, " ");
    
        if (strcmp(action, "LIN") == 0) {
            handleLoginRequest(buffer, response);
        } else if (strcmp(action, "LMA") == 0) {
            handleMyAuctionsRequest(buffer, response);
        } else if (strcmp(action, "LMB") == 0) {
            handleMyBidsRequest(buffer, response);
        } else if (strcmp(action, "LST") == 0) {
            handleListAuctionsRequest(response);
        } else if (strcmp(action, "SRC") == 0) {
            handleShowRecordRequest(buffer, response);
        } else if (strcmp(action, "LOU") == 0) {
            handleLogoutRequest(buffer, response);
        } else if (strcmp(action, "UNR") == 0) {
            handleUnregisterRequest(buffer, response);
        } else {
            *response = strdup("Unknown action"); //TODO
        }

        //sendUDPMessage(response, reply, ASIP, ASPort); TODO
    }

    close(fd);
}

void handleTCPRequests(char* ASPort) {
    int fd, newfd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd = createTCPSocket();

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, ASPort, &hints, &res);
    if (errcode != 0) {
        perror("Error in getaddrinfo");
        exit(EXIT_FAILURE);
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 5) == -1) {
        perror("Error listening on TCP socket");
        exit(EXIT_FAILURE);
    }

    printf("TCP server is listening on port %s...\n", ASPort);

    while (1) {
        addrlen = sizeof(addr);
        newfd = accept(fd, (struct sockaddr *)&addr, &addrlen);
        if (newfd == -1) {
            perror("Error accepting TCP connection");
            exit(EXIT_FAILURE);
        }

        n = read(newfd, buffer, sizeof(buffer));
        if (n == -1) {
            perror("Error reading from TCP socket");
            exit(EXIT_FAILURE);
        }

        //TODO

        close(newfd);
    }

    freeaddrinfo(res);
    close(fd);
}

void createDirectory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir(path, 0700);
    }
}

void createFile(const char *directory, const char *filename) {
    char filePath[100];
    snprintf(filePath, sizeof(filePath), "%s/%s", directory, filename);
    
    FILE *file = fopen(filePath, "w");
    
    if (file != NULL) {
        fclose(file);
    } else {
        perror("Error creating file"); //TODO
    }
}

void removeFile(const char* filePath) {
    if (remove(filePath) != 0) {
        perror("Error removing file");
    }
}

void createUserDirectory(const char* UID) {
    char userDir[50];
    snprintf(userDir, sizeof(userDir), "USERS/%s", UID);
    createDirectory(userDir);

    createFile(userDir, "pass.txt");
    createFile(userDir, "login.txt");

    createDirectory("HOSTED");
    createDirectory("BIDDED");
}

void createAuctionDirectory(int AID, const char* name, const char* asset_fname, int start_value, int timeactive) {
    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AUCTIONS/%d", AID);
    createDirectory(auctionDir);

    createFile(auctionDir, "START.txt");

    createFile(auctionDir, asset_fname);

    char bidsDir[50];
    snprintf(bidsDir, sizeof(bidsDir), "%s/BIDS", auctionDir);
    createDirectory(bidsDir);
}

void closeAuctionDirectory(int AID) {
    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AUCTIONS/%d", AID);

    createFile(auctionDir, "END.txt");
}

void addBid(const char* UID, int AID, int value) {
    char bidFile[50];
    snprintf(bidFile, sizeof(bidFile), "AUCTIONS/%d/BIDS/%06d", AID, value);
    createFile(bidFile, "");
}

void unregisterUser(const char* UID) {
    char passFile[50];
    snprintf(passFile, sizeof(passFile), "USERS/%s/pass.txt", UID);
    removeFile(passFile);

    char loginFile[50];
    snprintf(loginFile, sizeof(loginFile), "USERS/%s/login.txt", UID);
    removeFile(loginFile);
}