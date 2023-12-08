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

#define VALID_USER 0
#define USER_NOT_EXIST 1
#define INVALID_PASSWORD 2

void handleUDPRequests(char* ASPort, int verbose);
void handleTCPRequests(char* ASPort, int verbose);

void createDirectory(const char *path);
void createFile(const char *directory, const char *filename);
void writeFile(const char *filePath, const char *content);
void removeFile(const char *filePath);

int validateUser(const char* UID, const char* password);

void handleLoginRequest(char* request, char* response, int verbose);
void handleOpenAuctionRequest(char* request, char* response, int verbose);
void handleCloseAuctionRequest(char* request, char* response, int verbose);
void handleMyAuctionsRequest(char* request, char* response, int verbose);
void handleMyBidsRequest(char* request, char* response, int verbose);
void handleListAuctionsRequest(char** response, int verbose);
void handleShowAssetRequest(char* request, char* response, int verbose);
void handleBidRequest(char* request, char* response, int verbose);
void handleShowRecordRequest(char* request, char* response, int verbose);
void handleLogoutRequest(char* request, char* response, int verbose);
void handleUnregisterRequest(char* request, char* response, int verbose);


int main(int argc, char *argv[]) { //TODO take arguments
    pid_t udpProcess, tcpProcess;
    int opt;
    char ASport[6] = "58000"; // TODO: Default value GN
    int verbose = 0; // Verbose mode flag

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
        case 'p':
            snprintf(ASport, sizeof(ASport), "%s", optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-p ASport] [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Create UDP process
    udpProcess = fork();
    if (udpProcess < 0) {
        perror("Error forking UDP process");
        exit(EXIT_FAILURE);
    } else if (udpProcess == 0) {
        handleUDPRequests(ASport, verbose);
        exit(EXIT_SUCCESS);
    }

    // Create TCP process
    tcpProcess = fork();
    if (tcpProcess < 0) {
        perror("Error forking TCP process");
        exit(EXIT_FAILURE);
    } else if (tcpProcess == 0) {
        handleTCPRequests(ASport, verbose);
        exit(EXIT_SUCCESS);
    }

    // Wait for both child processes to finish
    waitpid(udpProcess, NULL, 0);
    waitpid(tcpProcess, NULL, 0);

    return 0;
}

void handleUDPRequests(char* ASPort, int verbose) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];
    char response[MAX_BUFFER_SIZE];

    fd = createUDPSocket();

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

    freeaddrinfo(res);
    addrlen = sizeof(addr);
    // Loop to receive and process UDP messages
    while (1) {
        // Receive data from the client
        n = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addrlen);
        if (n == -1) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        char* action = strtok(buffer, " ");
        char* data = strtok(NULL, "\n");
    
        if (strcmp(action, "LIN") == 0) {
            handleLoginRequest(data, response, verbose);
        } else if (strcmp(action, "LMA") == 0) {
            handleMyAuctionsRequest(data, response, verbose);
        } else if (strcmp(action, "LMB") == 0) {
            handleMyBidsRequest(data, response, verbose);
        } else if (strcmp(action, "LST") == 0) {
            handleListAuctionsRequest(response, verbose);
        } else if (strcmp(action, "SRC") == 0) {
            handleShowRecordRequest(data, response, verbose);
        } else if (strcmp(action, "LOU") == 0) {
            handleLogoutRequest(data, response, verbose);
        } else if (strcmp(action, "UNR") == 0) {
            handleUnregisterRequest(data, response, verbose);
        } else {
            //TODO: UNKNOWN
        }

        /* Envia a resposta para o endereço `addr` de onde foram recebidos dados */
        n = sendto(fd, response, sizeof(response), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            exit(1); //TODO: Better handling
        }
    }

    close(fd);
}

void handleTCPRequests(char* ASPort, int verbose) {
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
        exit(EXIT_FAILURE); //TODO: Better handling in all below
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Error binding TCP socket");
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 5) == -1) {
        perror("Error listening on TCP socket");
        exit(EXIT_FAILURE);
    }

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
            handleLoginRequest(buffer, response, verbose);
        } else if (strcmp(action, "OPA") == 0) {
            handleOpenAuctionRequest(buffer, response, verbose);
        } else if (strcmp(action, "CLS") == 0) {
            handleCloseAuctionRequest(buffer, response, verbose);
        } else if (strcmp(action, "SAS") == 0) {
            handleShowAssetRequest(buffer, response, verbose);
        } else if (strcmp(action, "BID") == 0) {
            handleBidRequest(buffer, response, verbose);
        } else {
            //TODO: UNKNOWN
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


/* --------------------- SUPPORT FUNCTIONS -----------------------------------*/

void createDirectory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            perror("Error creating directory");
            exit(EXIT_FAILURE);
        }
    }
}

void createFile(const char *directory, const char *filename) {
    char filePath[100];
    snprintf(filePath, sizeof(filePath), "%s/%s", directory, filename);
    
    FILE *file = fopen(filePath, "w");
    
    if (file != NULL) {
        fclose(file);
    } else {
        perror("Error creating file");
        exit(EXIT_FAILURE);
    }
}

void writeFile(const char *filePath, const char *content) {
    FILE *file = fopen(filePath, "w");
    
    if (file != NULL) {
        fprintf(file, "%s", content);
        fclose(file);
    } else {
        perror("Error writing to file");
        exit(EXIT_FAILURE);
    }
}

void removeFile(const char *filePath) {
    if (remove(filePath) != 0) {
        perror("Error removing file");
        exit(EXIT_FAILURE);
    }
}

/* --------------------- HELPER FUNCTIONS  -----------------------------------*/
// Helper function to validate user existence and password
int validateUser(const char* UID, const char* password) {
    char userDir[50];
    snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
    char filename[50];
    snprintf(filename, sizeof(filename), "%s_login.txt", UID);

    // Check if the user directory exists
    struct stat st;
    if (stat(userDir, &st) == 0) {
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        // Check if the login file exists
        if (access(filePath, F_OK) != -1) {
            // Check if the provided password matches the stored password
            snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            FILE* file = fopen(filePath, "r");
            if (file != NULL) {
                char storedPassword[MAX_BUFFER_SIZE];
                fscanf(file, "%s", storedPassword);
                fclose(file);

                return (strcmp(storedPassword, password) == 0) ? VALID_USER : INVALID_PASSWORD;
            }
        } else {
            // User exists, but login file is missing
            return INVALID_PASSWORD;
        }
    } else {
        // User does not exist
        return USER_NOT_EXIST;
    }

    return USER_NOT_EXIST;  // Default to USER_NOT_EXIST in case of unexpected errors
}

/* --------------------- HANDLER FUNCTIONS  -----------------------------------*/

void handleLoginRequest(char* request, char* response, int verbose) {
    char userDir[50];
    char filename[50];

    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password);

    if (validation == VALID_USER) {
        // User is already registered and password is correct, log in
        snprintf(response, MAX_BUFFER_SIZE, "RLI OK\n");
        return;
    } else if (validation == USER_NOT_EXIST) {
        // User is not registered, register and log in the user

        // Construct user directory path
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);

        // Create user directory
        createDirectory(userDir);

        // Create password file
        snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
        createFile(userDir, filename);

        // Create login file
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        createFile(userDir, filename);

        // Store the password in the password file
        snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        writeFile(filePath, password);

        // Create HOSTED and BIDDED directories
        createDirectory(strcat(userDir, "/HOSTED"));
        createDirectory(strcat(userDir, "/BIDDED"));

        snprintf(response, MAX_BUFFER_SIZE, "RLI REG\n");
        return;
    } else {
        // User exists but the password is invalid
        snprintf(response, MAX_BUFFER_SIZE, "RLI NOK\n");
        return;
    }
}

void handleOpenAuctionRequest(char* request, char* response, int verbose) {
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
        char auctionDir[50];
        snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);
        createDirectory(auctionDir);

        createFile(auctionDir, "START.txt");

        createFile(auctionDir, asset_fname);

        char bidsDir[50];
        snprintf(bidsDir, sizeof(bidsDir), "%s/BIDS", auctionDir);
        createDirectory(bidsDir);
        strcpy(response, "ROA OK [AID]");
    } else {
        strcpy(response, "ROA NOK");
    }
}

void handleCloseAuctionRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");

    int AID = atoi(AID_str);

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        char auctionDir[50];
        snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);

        createFile(auctionDir, "END.txt");
        strcpy(response, "RCL OK");
    } else {
        strcpy(response, "RCL NOK");
    }
}

void handleMyAuctionsRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");

    // TODO: Check if UID is valid
    if (/* UID is valid */) {
        // TODO: Implement logic to get a list of user's auctions
        strcpy(response, "RMA OK [AID state]*");
    } else {
        strcpy(response, "RMA NOK");
    }
}

void handleMyBidsRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");

    // TODO: Check if UID is valid
    if (/* UID is valid */) {
        // TODO: Implement logic to get a list of user's bids
        strcpy(response, "RMB OK [AID state]*");
    } else {
        strcpy(response, "RMB NOK");
    }
}

void handleListAuctionsRequest(char** response, int verbose) {
    // TODO: Implement logic to get a list of all auctions
    strcpy(*response, "RLS OK [AID state]*");
}

void handleShowAssetRequest(char* request, char* response, int verbose) {
    char* AID_str = strtok(NULL, " ");
    int AID = atoi(AID_str);

    // TODO: Implement logic to send the asset file
    strcpy(response, "RSA OK [Fname Fsize Fdata]");
}

void handleBidRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");
    char* value_str = strtok(NULL, " ");

    int AID = atoi(AID_str);
    int value = atoi(value_str);

    // TODO: Check if UID and password are valid
    if (/* UID and password are valid */) {
        char bidFile[50];
        snprintf(bidFile, sizeof(bidFile), "AS/AUCTIONS/%d/BIDS/%06d", AID, value);
        createFile(bidFile, "");
        strcpy(response, "RBD OK");
    } else {
        strcpy(response, "RBD NOK");
    }
}

void handleShowRecordRequest(char* request, char* response, int verbose) {
    char* AID_str = strtok(NULL, " ");
    int AID = atoi(AID_str);

    // TODO: Implement logic to get the auction record
    strcpy(response, "RRC OK [host_UID auction_name asset_fname start_value start_date-time timeactive] [B bidder_UID bid_value bid_date-time bid_sec_time]* [E end_date-time end_sec_time]");
}

void handleLogoutRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password);
    if (validation == VALID_USER) {
        // User is logged out
        char filename[50];
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        char userDir[50];
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);

        // Delete the login file
        removeFile(filePath);

        strcpy(response, "RLO OK");
    } else if (validation == INVALID_PASSWORD) {// or if was not logged in
        strcpy(response, "RLO NOK");
    } else if (validation == USER_NOT_EXIST) {
        strcpy(response, "RLO UNR");
    }
}

void handleUnregisterRequest(char* request, char* response, int verbose) {
    char* UID = strtok(NULL, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password);

    if (validation == VALID_USER) {
        // User exists and password is correct, unregister the user
        // Delete password file
        char passFile[50];
        snprintf(passFile, sizeof(passFile), "AS/USERS/%s_pass.txt", UID);
        removeFile(passFile);

        // Delete login file
        char loginFile[50];
        snprintf(loginFile, sizeof(loginFile), "AS/USERS/%s_login.txt", UID);
        removeFile(loginFile);

        strcpy(response, "RUR OK");
    } else if (validation == USER_NOT_EXIST) {
        // User is not registered
        strcpy(response, "RUR UNR");
    } else {
        // User exists but the password is invalid
        strcpy(response, "RUR NOK");
    }
}
