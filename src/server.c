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
    char response[MAX_BUFFER_SIZE];  // Assuming MAX_BUFFER_SIZE is defined

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

        /* Envia a resposta para o endereço `addr` de onde foram recebidos dados */
        n = sendto(fd, response, sizeof(response), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            exit(1);
        }
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
    char response[MAX_BUFFER_SIZE];  // Assuming MAX_BUFFER_SIZE is defined

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

        char* action = strtok(buffer, " ");

        // Identify the type of request and call the appropriate handler function
        if (strcmp(action, "LIN") == 0) {
            handleLoginRequest(buffer, response);
        } else if (strcmp(action, "OPA") == 0) {
            handleOpenAuctionRequest(buffer, response);
        } else if (strcmp(action, "CLS") == 0) {
            handleCloseAuctionRequest(buffer, response);
        } else if (strcmp(action, "SAS") == 0) {
            handleShowAssetRequest(buffer, response);
        } else if (strcmp(action, "BID") == 0) {
            handleBidRequest(buffer, response);
        } else {
            snprintf(response, sizeof(response), "Unknown action"); //TODO
        }

        /* Envia a response para a socket */
        n = write(newfd, response, sizeof(response));
        if (n == -1) {
            exit(1);
        }
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
    snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
    createDirectory(userDir);

    createFile(userDir, "pass.txt");
    createFile(userDir, "login.txt");

    createDirectory("HOSTED");
    createDirectory("BIDDED");
}

void createAuctionDirectory(int AID, const char* name, const char* asset_fname, int start_value, int timeactive) {
    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);
    createDirectory(auctionDir);

    createFile(auctionDir, "START.txt");

    createFile(auctionDir, asset_fname);

    char bidsDir[50];
    snprintf(bidsDir, sizeof(bidsDir), "%s/BIDS", auctionDir);
    createDirectory(bidsDir);
}

void closeAuctionDirectory(int AID) {
    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);

    createFile(auctionDir, "END.txt");
}

void addBid(const char* UID, int AID, int value) {
    char bidFile[50];
    snprintf(bidFile, sizeof(bidFile), "AS/AUCTIONS/%d/BIDS/%06d", AID, value);
    createFile(bidFile, "");
}

void unregisterUser(const char* UID) {
    char passFile[50];
    snprintf(passFile, sizeof(passFile), "AS/USERS/%s/pass.txt", UID);
    removeFile(passFile);

    char loginFile[50];
    snprintf(loginFile, sizeof(loginFile), "AS/USERS/%s/login.txt", UID);
    removeFile(loginFile);
}

// Include necessary headers

void handleLoginRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        createUserDirectory(UID);
        strcpy(response, "RLI OK");
    } else {
        strcpy(response, "RLI NOK");
    }
}

void handleOpenAuctionRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* name = strtok(NULL, " ");
    char* start_value_str = strtok(NULL, " ");
    char* timeactive_str = strtok(NULL, " ");
    char* Fname = strtok(NULL, " ");
    char* Fsize_str = strtok(NULL, " ");
    char* Fdata = strtok(NULL, " ");

    int start_value = atoi(start_value_str);
    int timeactive = atoi(timeactive_str);
    int Fsize = atoi(Fsize_str);

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        // TODO: Generate AID and create auction directory
        createAuctionDirectory(AID, name, Fname, start_value, timeactive);
        strcpy(response, "ROA OK [AID]");
    } else {
        strcpy(response, "ROA NOK");
    }
}

void handleCloseAuctionRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");

    int AID = atoi(AID_str);

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        // TODO: Close the auction directory
        closeAuctionDirectory(AID);
        strcpy(response, "RCL OK");
    } else {
        strcpy(response, "RCL NOK");
    }
}

void handleMyAuctionsRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");

    // TODO: Check if UID is valid
    if (/* UID is valid */) {
        // TODO: Implement logic to get a list of user's auctions
        strcpy(response, "RMA OK [AID state]*");
    } else {
        strcpy(response, "RMA NOK");
    }
}

void handleMyBidsRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");

    // TODO: Check if UID is valid
    if (/* UID is valid */) {
        // TODO: Implement logic to get a list of user's bids
        strcpy(response, "RMB OK [AID state]*");
    } else {
        strcpy(response, "RMB NOK");
    }
}

void handleListAuctionsRequest(char** response) {
    // TODO: Implement logic to get a list of all auctions
    strcpy(*response, "RLS OK [AID state]*");
}

void handleShowAssetRequest(char* request, char* response) {
    char* AID_str = strtok(NULL, " ");
    int AID = atoi(AID_str);

    // TODO: Implement logic to send the asset file
    strcpy(response, "RSA OK [Fname Fsize Fdata]");
}

void handleBidRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");
    char* value_str = strtok(NULL, " ");

    int AID = atoi(AID_str);
    int value = atoi(value_str);

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        // TODO: Implement logic to place a bid
        strcpy(response, "RBD OK");
    } else {
        strcpy(response, "RBD NOK");
    }
}

void handleShowRecordRequest(char* request, char* response) {
    char* AID_str = strtok(NULL, " ");
    int AID = atoi(AID_str);

    // TODO: Implement logic to get the auction record
    strcpy(response, "RRC OK [host_UID auction_name asset_fname start_value start_date-time timeactive] [B bidder_UID bid_value bid_date-time bid_sec_time]* [E end_date-time end_sec_time]");
}

void handleLogoutRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        // TODO: Implement logic to logout the user
        strcpy(response, "RLO OK");
    } else {
        strcpy(response, "RLO NOK");
    }
}

void handleUnregisterRequest(char* request, char* response) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        // TODO: Implement logic to unregister the user
        strcpy(response, "RUN OK");
    } else {
        strcpy(response, "RUN NOK");
    }
}
