#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

// #include "common.h"

#define VALID_USER 0
#define USER_NOT_EXIST 1
#define INVALID_PASSWORD 2
#define MISSING_PASSWORD 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MAX_BUFFER_SIZE 4000
#define UID_SIZE 6
#define PASSWORD_SIZE 8
#define MAX_FILENAME_SIZE 24
#define MAX_FSIZE_LEN 8
#define MAX_FSIZE_NUM 0xA00000 // 10 MB

int createUDPSocket() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        perror("UDP socket creation failed");
        return-1;
    }
    return udpSocket;
}

int createTCPSocket() {
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1) {
        perror("TCP socket creation failed");
        return-1;
    }
    return tcpSocket;
}

int handleUDPRequests(char* ASport, int verbose);
int handleTCPRequests(char* ASport, int verbose);

int createDirectory(const char *path);
int createFile(const char *directory, const char *filename);
int writeFile(const char *filePath, const char *content);
int removeFile(const char *filePath);
void getCurrentDateTime(char* dateTime);

int validateUser(const char* UID, const char* password);
int newAIDdirectory();
bool auctionExists(int AID);
bool auctionOwnedByUser(int AID, const char* UID);
bool auctionAlreadyEnded(int AID);
int newBid(int AID, int newBidValue);
int validateUserLoggedIn(const char* UID);
int isDirectoryEmpty(const char* path);
int hasAuctionEnded(int AID);
int fileExists(const char* filePath);

void handleLoginRequest(char* request, char* response, int verbose);
void handleOpenAuctionRequest(char* request, char* response, int verbose);
void handleCloseAuctionRequest(char* request, char* response, int verbose);
void handleMyAuctionsRequest(char* request, char* response, int verbose);
void handleMyBidsRequest(char* request, char* response, int verbose);
void handleListAuctionsRequest(char* response, int verbose);
void handleShowAssetRequest(char* request, char* response, int verbose);
void handleBidRequest(char* request, char* response, int verbose);
void handleShowRecordRequest(char* request, char* response, int verbose);
void handleLogoutRequest(char* request, char* response, int verbose);
void handleUnregisterRequest(char* request, char* response, int verbose);

volatile sig_atomic_t terminationRequested = 0;

void signalHandler(int signum) {
    switch(signum) {
        case SIGINT:
            terminationRequested = 1;
            printf("\nReceived SIGINT, terminating server...\n");
            break;
        case SIGTSTP:
            terminationRequested = 1;
            printf("\nReceived SIGTSTP, pausing server...\n");
            break;
    }
}

int main(int argc, char *argv[]) {
    int opt;
    char ASport[6] = "58022";
    int verbose = 0; // Verbose mode flag

     // Set up signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;

    // Register SIGINT handler
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        return EXIT_FAILURE;
    }

    // Register SIGTSTP handler
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("Error setting up SIGTSTP handler");
        return EXIT_FAILURE;
    }

    //TODO: Confirm this stuff
    struct sigaction act; //TODO: SIGPIPE signal will be ignored
                          //TODO: if the connection is lost and you write to the socket, the write will return -1 and errno will be set to EPIPE
    memset(&act,0,sizeof act);
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL)==-1)exit(EXIT_FAILURE);
    if(sigaction(SIGCHLD,&act,NULL)==-1)exit(EXIT_FAILURE);

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
        case 'p':
            // If port number is provided, update ASport
            snprintf(ASport, sizeof(ASport), "%s", optarg);
            break;
        case 'v':
            // If verbose flag is provided, set verbose to 1
            verbose = 1;
            break;
        default:
            // If invalid arguments are provided, print usage and exit
            fprintf(stderr, "Usage: %s [-p ASport] [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("To Terminate use CTRL+C or CTRL+Z\n");

    //TODO: Confirmar que pastas certas estão criadas? senão criar?
    
    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // If fork fails, print error and exit
        perror("Failed to fork into UDP and TCP listeners");
        exit(EXIT_FAILURE);
    } else if (pid != 0) {
        // Parent process
        handleUDPRequests(ASport, verbose);
    } else {
        // Child process
        if(handleTCPRequests(ASport, verbose) == -1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    // Wait for the child process to finish
    int status;
    waitpid(pid, &status, 0);

    return EXIT_SUCCESS;
}

int handleUDPRequests(char* ASport, int verbose) {
    // Variable declarations
    int errcode;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char udpBuffer[MAX_BUFFER_SIZE];//should this be as big as MAX_BUFFER_SIZE?
    char response[MAX_BUFFER_SIZE];

    // Create a UDP socket
    int udpSocket = createUDPSocket();
    if (udpSocket == -1) {
        return -1;
    }

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info
    errcode = getaddrinfo(NULL, ASport, &hints, &res);
    if (errcode != 0) {
        // If getaddrinfo fails, print error and exit
        perror("getaddrinfo failed");
        return -1;
    }

    // Bind the socket to the specified address
    ssize_t n = bind(udpSocket, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        // If bind fails, print error and exit
        perror("Error binding UDP socket:");
        freeaddrinfo(res);
        return -1;
    }

    // Free the address info struct
    freeaddrinfo(res);

    // Set up the address length
    addrlen = sizeof(addr);

    // If verbose mode is on, print the listening port
    if (verbose) {
        printf("[UDP] Listening on port %s\n", ASport);
    }

    // Main loop to handle incoming requests
    while (!terminationRequested) {
        fd_set readSet;
        struct timeval timeout;
        FD_ZERO(&readSet);
        FD_SET(udpSocket, &readSet);

        // Set a timeout for select to periodically check for termination
        timeout.tv_sec = 1; 
        timeout.tv_usec = 0;

        // Use select to periodically check for termination
        int result = select(udpSocket + 1, &readSet, NULL, NULL, &timeout);

        if (result == -1) {
            // Handle select error if necessary
            perror("select");
            break;
        } else if (result == 0) {
            // Timeout occurred, continue the loop
            continue;
        }
        // Receive data from the client
        n = recvfrom(udpSocket, udpBuffer, sizeof(udpBuffer), 0, (struct sockaddr *)&addr, &addrlen);
        if (n <= 0) {
            // If recvfrom fails, print error and continue to the next iteration
            perror("Error receiving UDP message");
            continue;
        }
        // Null-terminate the received data
        udpBuffer[n] = '\0';
        if(verbose == 1){
                printf("[TCP] Received: %s\n", udpBuffer);
            }
        // Process the received data
        if (udpBuffer[n - 1] != '\n') {//TODO: por igual no TCP
            // If the last character is not a newline, send an error response
            sprintf(response, "ERR\n"); 
        } else {
            // Otherwise, parse the action and data from the received message
            char* action = strtok(udpBuffer, " ");
            char* data = strtok(NULL, "\n");

            // Handle the action
            if (strcmp(action, "LIN") == 0) {
                handleLoginRequest(data, response, verbose);
            } else if (strcmp(action, "LMA") == 0) {
                handleMyAuctionsRequest(data, response, verbose);
            } else if (strcmp(action, "LMB") == 0) {
                handleMyBidsRequest(data, response, verbose);
            } else if (strcmp(action, "LST\n") == 0) {
                handleListAuctionsRequest(response, verbose);
            } else if (strcmp(action, "SRC") == 0) {
                handleShowRecordRequest(data, response, verbose);
            } else if (strcmp(action, "LOU") == 0) {
                handleLogoutRequest(data, response, verbose);
            } else if (strcmp(action, "UNR") == 0) {
                handleUnregisterRequest(data, response, verbose);
            } else {
                // If the action is unknown, send an error response
                sprintf(response, "ERR\n"); 
            }
        }
        // Send the response to the client
        //char* newline_pos = strchr(response, '\n');
        n = sendto(udpSocket, response, sizeof(response), 0, (struct sockaddr *)&addr, addrlen);
        if (n == -1) {
            // If sendto fails, print error and continue to the next iteration
            perror("Error sending UDP message");
            continue;
        }
    }
    // Close the socket
    close(udpSocket);
    return 0;
}

int handleTCPRequests(char* ASport, int verbose){
    // Variable declarations
    int errcode;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[MAX_BUFFER_SIZE];
    char response[MAX_BUFFER_SIZE];

    // Create a TCP socket
    int tcpSocket = createTCPSocket();
    if (tcpSocket == -1) {
        return -1;
    }

    // TODO: check if this is needed Set the SO_REUSEADDR option
    int yes = 1;
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set up hints for getaddrinfo
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info
    errcode = getaddrinfo(NULL, ASport, &hints, &res);
    if (errcode != 0) {
        // If getaddrinfo fails, print error and exit
        perror("Error in getaddrinfo");
        return -1;
    }

    // Bind the socket to the specified address
    if (bind(tcpSocket, res->ai_addr, res->ai_addrlen) == -1) {
        // If bind fails, print error and exit
        perror("Error binding TCP socket");
        freeaddrinfo(res);
        return -1;
    }
    // Free the address info struct
    freeaddrinfo(res);

    // Listen for incoming connections
    if (listen(tcpSocket, 5) == -1) { //TODO: only 5 waiting requests?
        // If listen fails, print error and exit
        perror("Error listening on TCP socket");
        return -1;
    }

    // Set up the address length
    addrlen = sizeof(addr);

    if (verbose) {
        printf("[TCP] Listening on port %s\n", ASport);
    }

    // Main loop to handle incoming connections
    while (!terminationRequested) {
        fd_set readSet;
        struct timeval timeout;
        FD_ZERO(&readSet);
        FD_SET(tcpSocket, &readSet);

        // Set a timeout for select to periodically check for termination
        timeout.tv_sec = 1; // Adjust this value based on your requirements
        timeout.tv_usec = 0;

        // Use select to periodically check for termination
        int result = select(tcpSocket + 1, &readSet, NULL, NULL, &timeout);

        if (result == -1) {
            // Handle select error if necessary
            perror("select");
            break;
        } else if (result == 0) {
            // Timeout occurred, continue the loop
            continue;
        }
        // Accept a new connection
        int sessionFd  = accept(tcpSocket, (struct sockaddr *)&addr, &addrlen);

        if (sessionFd == -1) {
            // If accept fails, print error and continue to the next iteration
            perror("Error accepting TCP connection");
            continue;
        }

        // Fork a new process to handle the connection
        pid_t tcpPid = fork();

        if (tcpPid == -1) {
            // If fork fails, print error, close the connection, and continue to the next iteration
            perror("Failed to fork TCP Session");
            close(sessionFd);
            continue;
        } else if (tcpPid != 0) {
            // Child process
            // Close the listening socket
            close(tcpSocket);
            // Read data from the client
            size_t alreadyRead = 0;
            ssize_t n;
            while (alreadyRead < MAX_BUFFER_SIZE && (n = read(sessionFd, buffer + alreadyRead, sizeof(char))) > 0) {
                alreadyRead += (size_t)n;
                if (buffer[alreadyRead - 1] == '\n') {
                    break;
                }
            }
            if(verbose == 1){
                printf("[TCP] Received: %s\n", buffer);
            }
            // Parse the action from the received data
            char* action = strtok(buffer, " ");
            char* data = strtok(NULL, "\n");

            // Handle the action
            if (strcmp(action, "LIN") == 0) {
                handleLoginRequest(data, response, verbose);
            } else if (strcmp(action, "OPA") == 0) {
                handleOpenAuctionRequest(data, response, verbose);
            } else if (strcmp(action, "CLS") == 0) {
                handleCloseAuctionRequest(data, response, verbose);
            } else if (strcmp(action, "SAS") == 0) {
                handleShowAssetRequest(data, response, verbose);
            } else if (strcmp(action, "BID") == 0) {
                handleBidRequest(data, response, verbose);
            } else {
                // If the action is unknown, send an error response
                sprintf(response, "ERR\n"); 
            }

            // Send the response to the client
            size_t toWrite = strlen(response);
            size_t written = 0;
            while (written < toWrite) {
                ssize_t sent = write(sessionFd, response + written, MIN(MAX_BUFFER_SIZE, toWrite - written));
                if (sent <= 0) {
                    // If write fails, print error and break the loop
                    perror("Failed to write to to TCP Socket");
                    break;
                }
                written += (size_t)sent;
            }
            // Close the connection and exit
            close(sessionFd);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            // Close the connection and continue to the next iteration
            close(sessionFd);
            continue;
        }
    }
    close(tcpSocket);
    return 0;
}

/* --------------------- SUPPORT FUNCTIONS -----------------------------------*/

int createDirectory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) == -1) {
            perror("Error creating directory");
            return -1;
        }
    }
    return 0;
}

int createFile(const char *directory, const char *filename) {
    char filePath[100];
    snprintf(filePath, sizeof(filePath), "%s/%s", directory, filename);

    
    FILE *file = fopen(filePath, "w");
    
    if (file != NULL) {
        fclose(file);
    } else {
        perror("Error creating file");
        return -1;
    }
    return 0;
}

int writeFile(const char *filePath, const char *content) {
    FILE *file = fopen(filePath, "w");
    
    if (file != NULL) {
        fprintf(file, "%s", content);
        fclose(file);
    } else {
        perror("Error writing to file");
        return -1;
    }
    return 0;
}

int removeFile(const char *filePath) {
    if (remove(filePath) != 0) {
        perror("Error removing file");
        return -1;
    }
    return 0;
}

// Function to get the current date and time in the required format
void getCurrentDateTime(char* dateTime) {
    time_t t;
    struct tm* tmp;

    t = time(NULL);
    tmp = localtime(&t);

    strftime(dateTime, 20, "%Y-%m-%d %H:%M:%S", tmp);
}
/* --------------------- HELPER FUNCTIONS  -----------------------------------*/
// Helper function to validate user existence and password
int validateUser(const char* UID, const char* password) {
    char userDir[50];
    snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
    char filename[50];
    snprintf(filename, sizeof(filename), "%s_pass.txt", UID);

    // Check if the user directory exists
    struct stat st;
    if (stat(userDir, &st) == 0) {
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        // Check if the pass file exists
        if (access(filePath, F_OK) != -1) {
            // Check if the provided password matches the stored password
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            FILE* file = fopen(filePath, "r");
            if (file != NULL) {
                char storedPassword[MAX_BUFFER_SIZE];
                fscanf(file, "%s", storedPassword);
                fclose(file);
                return (strcmp(storedPassword, password) == 0) ? VALID_USER : INVALID_PASSWORD;
            }
        } else {
            // User exists, but pass file is missing
            return MISSING_PASSWORD;
        }
    } else {
        // User does not exist
        return USER_NOT_EXIST;
    }

    return -1; // Unexpected error
}

// Helper function to find the highest AID in the AUCTIONS directory and create a new directory for the next AID
int newAIDdirectory() {
    DIR *dir = opendir("AS/AUCTIONS");
    struct dirent *entry;
    int highestAID = 0;

    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                int currentAID = atoi(entry->d_name);
                if (currentAID > highestAID) {
                    highestAID = currentAID;
                }
            }
        }

        // Check if the next AID would be too large
        int nextAID = highestAID + 1;
        if (nextAID > 999) {
            fprintf(stderr, "Error: reached limit of 999 auctions\n");
            closedir(dir);
            return -1;
        }

        // Create the directory for the next AID
        char auctionDir[50];
        snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%03d", nextAID);
        if(createDirectory(auctionDir) == -1){
            closedir(dir);
            return -1;
        }
        closedir(dir);
        return nextAID;
    } else {
        perror("Error opening AUCTIONS directory");
        return -1;
    }
}

// Helper function to check if an auction with the given AID exists
bool auctionExists(int AID) {
    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);

    struct stat st;
    return stat(auctionDir, &st) == 0 && S_ISDIR(st.st_mode);
}

// Helper function to check if an auction with the given AID is owned by the specified UID
bool auctionOwnedByUser(int AID, const char* UID) {
    char hostedDir[50];
    snprintf(hostedDir, sizeof(hostedDir), "AS/USERS/%s/HOSTED", UID);

    char auctionFile[100];
    snprintf(auctionFile, sizeof(auctionFile), "%s/%d.txt", hostedDir, AID);

    return access(auctionFile, F_OK) != -1;
}

// Helper function to check if an auction with the given AID has already ended
bool auctionAlreadyEnded(int AID) {
    char endFile[100];
    snprintf(endFile, sizeof(endFile), "AS/AUCTIONS/%d/END.txt", AID);

    return access(endFile, F_OK) != -1;
}


// Helper function to create a new bid if possible
int newBid(int AID, int newBidValue) {
    char bidsDir[50];
    snprintf(bidsDir, sizeof(bidsDir), "AS/AUCTIONS/%d/BIDS", AID);

    DIR* dir = opendir(bidsDir);
    struct dirent* entry;
    int highestBid = 0;

    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {  // Regular file
                int currentBid = atoi(entry->d_name);
                if (currentBid > highestBid) {
                    highestBid = currentBid;
                }
            }
        }

        closedir(dir);

        if (newBidValue > highestBid) {
            // Create a new entry for the bid
            char bidFilePath[50];
            snprintf(bidFilePath, sizeof(bidFilePath), "AS/AUCTIONS/%d/BIDS", AID);
            char bidFilename[50];
            snprintf(bidFilename, sizeof(bidFilename), "%06d.txt", newBidValue);
            return createFile(bidFilePath, bidFilename);
        } else {
            // Bid refused because a larger bid has already been placed
            return -2;
        }
    } else {
        perror("Error opening BIDS directory");
        return -1;
    }
}

// Helper function to validate if the user is logged in
int validateUserLoggedIn(const char* UID) {
    char loginFile[50];
    snprintf(loginFile, sizeof(loginFile), "AS/USERS/%s/%s_login.txt", UID, UID);

    // Check if the login file exists
    if (access(loginFile, F_OK) != -1) {
        return 1;  // User is logged in
    } else {
        return 0;  // User is not logged in
    }
}

// Helper function to check if a directory is empty
int isDirectoryEmpty(const char* path) {
    DIR* dir = opendir(path);
    if (dir != NULL) {
        struct dirent* entry = readdir(dir);
        int isEmpty = (entry == NULL);
        closedir(dir);
        return isEmpty;
    }
    return -1;  // Error opening directory
}

// Helper function to check if an auction has ended (based on the presence of END.txt file)
int hasAuctionEnded(int AID) {
    char endFilePath[100];
    snprintf(endFilePath, sizeof(endFilePath), "AS/AUCTIONS/%d/END.txt", AID);
    return access(endFilePath, F_OK) != -1;
}

// Helper function to check if a file exists
int fileExists(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (file != NULL) {
        fclose(file);
        return 1;  // File exists
    }
    return 0;  // File does not exist
}

/* --------------------- HANDLER FUNCTIONS  -----------------------------------*/

void handleLoginRequest(char* request, char* response, int verbose) {
    char userDir[50];
    char filename[50];
    char filePath[100];

    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password);//TODO: check if user is already logged in first?

    if (validation == VALID_USER || validation == MISSING_PASSWORD) {
        // User is already registered and password is correct, log in
        // Construct user directory path
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
        // Create login file
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        
        if(createFile(userDir, filename) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }
        
        if(validation == MISSING_PASSWORD){//if user was registered but had no password, assign new password
            snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
            if(createFile(userDir, filename) == -1){
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            if(writeFile(filePath, password) == -1){
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }
            
        }

        snprintf(response, MAX_BUFFER_SIZE, "RLI OK\n");

        return;
    } else if (validation == USER_NOT_EXIST) {
        // User is not registered, register and log in the user

        // Construct user directory path
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
        // Create user directory
        if(createDirectory(userDir) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }

        // Create password file
        snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
        if(createFile(userDir, filename) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }
        // Create login file
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        if(createFile(userDir, filename) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }
        
        // Store the password in the password file
        snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        if(writeFile(filePath, password) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }
        
        // Create HOSTED and BIDDED directories
        char userDirCopy[50];
        strcpy(userDirCopy, userDir);
        if(createDirectory(strcat(userDir, "/HOSTED")) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }

        if(createDirectory(strcat(userDirCopy, "/BIDDED")) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }

        snprintf(response, MAX_BUFFER_SIZE, "RLI REG\n");
        return;
    } else { // INVALID_PASSWORD
        // User exists but the password is invalid
        snprintf(response, MAX_BUFFER_SIZE, "RLI NOK\n");
        return;
    }
}

//TODO: como e quando encerrar o leilao com limite de tempo? -> requests que o tentem aceder
void handleOpenAuctionRequest(char *request, char *response, int verbose) {
    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char *UID = strtok(request, " ");
    char *password = strtok(NULL, " ");
    char *name = strtok(NULL, " ");
    char *start_value_str = strtok(NULL, " ");
    char *timeactive_str = strtok(NULL, " ");
    char *Fname = strtok(NULL, " ");
    char *Fsize_str = strtok(NULL, " ");
    (void)Fsize_str; // unused
    char *Fdata = strtok(NULL, "\n");

    //TODO: assert correct sizes and formats MAX_FILENAME_SIZE, MAX_FSIZE_LEN, MAX_FSIZE_NUM, UID_SIZE, PASSWORD_SIZE...

    int start_value = atoi(start_value_str);
    int timeactive = atoi(timeactive_str);
    //int Fsize = atoi(Fsize_str); //TODO: unused - remove?

    int validation = validateUser(UID, password); //TODO: check if user is already logged in first?

    if (validation == VALID_USER) {
        // Valid user
        int AID = newAIDdirectory();
        if (AID != -1) {
            // Create START file
            char startFilePath[100];
            snprintf(startFilePath, sizeof(startFilePath), "AS/AUCTIONS/%03d/START.txt", AID);
            FILE *startFile = fopen(startFilePath, "w");
            if (startFile != NULL) {
                fprintf(startFile, "%s %s %s %d %d", UID, name, Fname, start_value, timeactive);
                fclose(startFile);
            } else {
                perror("Error creating START file");
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }

            // Create asset file
            char assetFilePath[100];
            snprintf(assetFilePath, sizeof(assetFilePath), "AS/AUCTIONS/%03d/%s", AID, Fname);
            if(writeFile(assetFilePath, Fdata) == -1){
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }

            // Create BIDS directory
            char bidsDir[50];
            snprintf(bidsDir, sizeof(bidsDir), "AS/AUCTIONS/%03d/BIDS", AID);
            if(createDirectory(bidsDir) == -1){
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }

            // Create an empty file in the UID's HOSTED directory with the AID as the filename
            char hostedDir[50];
            snprintf(hostedDir, sizeof(hostedDir), "AS/USERS/%s/HOSTED", UID);
            if(createDirectory(hostedDir) == -1){
                snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                return;
            }

            char hostedFilename[100];
            snprintf(hostedFilename, sizeof(hostedFilename), "%03d.txt", AID);
            
            if(createFile(hostedDir, hostedFilename) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }

            snprintf(response, MAX_BUFFER_SIZE, "ROA OK %03d\n", AID);
        } else {
            // Error creating new AID directory
            snprintf(response, MAX_BUFFER_SIZE, "ROA NOK\n");
            return;
        }
    } else if (validation == USER_NOT_EXIST) {
        snprintf(response, MAX_BUFFER_SIZE, "ROA NLG\n");
    } else if (validation == INVALID_PASSWORD) {
        snprintf(response, MAX_BUFFER_SIZE, "ROA NLG\n");
    } else {
        // Unexpected error
        snprintf(response, MAX_BUFFER_SIZE, "ROA NOK\n");
    }
}

void handleCloseAuctionRequest(char* request, char* response, int verbose) {
    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");

    int AID = atoi(AID_str);

    // Check if UID and password are valid
    int validation = validateUser(UID, password); //TODO: check if user is already logged in first?

    if (validation == VALID_USER) {
        // Check if the auction exists
        if (auctionExists(AID)) {
            // Check if the auction is owned by the user
            if (auctionOwnedByUser(AID, UID)) {
                // Check if the auction has already ended
                if (!auctionAlreadyEnded(AID)) {
                    // Close the auction by creating the END file
                    char endFilePath[100];
                    snprintf(endFilePath, sizeof(endFilePath), "AS/AUCTIONS/%s", AID_str);
                    if(createFile(endFilePath, "END.txt") == -1){
                        snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
                        return;
                    }
                    strcpy(response, "RCL OK\n");
                    return;
                } else {
                    strcpy(response, "RCL END\n"); // Auction has already ended
                }
            } else {
                strcpy(response, "RCL EOW\n"); // Auction is not owned by the user
            }
        } else {
            strcpy(response, "RCL EAU\n"); // Auction does not exist
        }
    } else if (validation == USER_NOT_EXIST) {
        strcpy(response, "RCL NLG\n"); //TODO: Not  specified? maybe NOK
    } else if (validation == INVALID_PASSWORD) {
        strcpy(response, "RCL NLG\n");
    } else {
        // Unexpected error
        strcpy(response, "RCL NOK\n");
    }
}

void handleMyAuctionsRequest(char* request, char* response, int verbose) {//TODO: resposta mal formatada, maybe overwritte da segunda parte
    char auctionString[10];
    int auctionCount = 0;

    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");

    // Initialize response with "RMA OK"
    snprintf(response, MAX_BUFFER_SIZE, "RMA OK");

    char userDir[50];
    snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);

    struct dirent* entry;
    DIR* hostedDirPtr = opendir(strcat(userDir, "/HOSTED"));

    if (hostedDirPtr != NULL) {
        // Loop through the HOSTED directory
        while ((entry = readdir(hostedDirPtr)) != NULL) {
            //TODO: ver se isto é preciso no if: entry->d_type == DT_DIR && 
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char* AID = entry->d_name;
                // Check if the auction has ended
                char state = hasAuctionEnded(atoi(AID)) ? '0' : '1'; //TODO: REMOVE?  (myauctions, mybids, list) vêm tanto auctions ativos como inativos
                // Append AID and state to the string
                snprintf(auctionString, sizeof(auctionString), " %s %c", AID, state);
                strncat(response, auctionString, MAX_BUFFER_SIZE - strlen(response) - 1);
                auctionCount++;
            }
        }
        closedir(hostedDirPtr);
    }

    // Check if the user has ongoing bids
    DIR* biddedDirPtr = opendir(strcat(userDir, "/BIDDED"));

    if (biddedDirPtr != NULL) {//TODO: Repeated code... just the directory change
        // Loop through the BIDDED directory
        while ((entry = readdir(biddedDirPtr)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char* AID = entry->d_name;
                // Check if the auction has ended
                char state = hasAuctionEnded(atoi(AID)) ? '0' : '1'; //TODO: REMOVE?  (myauctions, mybids, list) vêm tanto auctions ativos como inativos
                // Append AID and state to the string
                snprintf(auctionString, sizeof(auctionString), " %s %c", AID, state);
                strncat(response, auctionString, MAX_BUFFER_SIZE - strlen(response) - 1);
                auctionCount++;
            }
        }
        closedir(biddedDirPtr);
    }

    // Check if there are ongoing auctions for the user
    if (auctionCount > 0) {
        // At least one ongoing auction
        strcat(response, "\n");
    } else {
        // No ongoing auctions for the user
        snprintf(response, MAX_BUFFER_SIZE, "RMA NOK\n");
    }
}

void handleMyBidsRequest(char* request, char* response, int verbose) { //TODO: Diferent lkogic than other functions thatdo similar things...
    char bidString[10];
    int bidCount = 0;

    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");

    int loggedIn = validateUserLoggedIn(UID);

    if (loggedIn) {
        // Initialize response with "RMA OK"
        snprintf(response, MAX_BUFFER_SIZE, "RMB OK");

        char biddedDir[50];
        snprintf(biddedDir, sizeof(biddedDir), "AS/USERS/%s/BIDDED", UID);

        if (isDirectoryEmpty(biddedDir) == 0) {
            // User has ongoing bids
            // Prepare response string
            snprintf(response, MAX_BUFFER_SIZE, "RMB OK");

            // Open the BIDDED directory
            DIR* biddedDirPtr = opendir(biddedDir);

            if (biddedDirPtr != NULL) {
                struct dirent* entry;

                // Loop through the BIDDED directory
                while ((entry = readdir(biddedDirPtr)) != NULL) {
                    if (entry->d_type == DT_REG) {  // Regular file (assuming each entry is a file)
                        char* AID = entry->d_name;
                        // Check if the auction has ended
                        char state = hasAuctionEnded(atoi(AID)) ? '0' : '1';//TODO: REMOVE?  (myauctions, mybids, list) vêm tanto auctions ativos como inativos

                        // Append AID and state to the response string
                        snprintf(bidString, sizeof(bidString), " %s %c", AID, state);
                        strncat(response, bidString, MAX_BUFFER_SIZE - strlen(response) - 1);
                        bidCount++;
                    }
                }
                closedir(biddedDirPtr);
                strcat(response, "\n");
            }else {
                //BIDS Directory not found
                snprintf(response, MAX_BUFFER_SIZE, "RMB NOK\n");
            }
        } else {
            // User has no ongoing bids
            snprintf(response, MAX_BUFFER_SIZE, "RMB NOK\n");
            return;
        }

        if(bidCount==0){

            snprintf(response, MAX_BUFFER_SIZE, "RMB NOK\n");

        }
    } else {
        // User is not logged in
        strcpy(response, "RMB NLG\n");
    }
}

void handleListAuctionsRequest(char* response, int verbose) {
    char auctionString[10];
    int auctionCount = 0;

    if (verbose) {
        printf("Request received: LST\n");
    }

    // Initialize response with "LST OK"
    snprintf(response, MAX_BUFFER_SIZE, "LST OK");
    
    // Open the AUCTIONS directory
    DIR* auctionsDirPtr = opendir("AS/AUCTIONS");

    if (auctionsDirPtr != NULL) {
        struct dirent* entry;
        // Loop through the AUCTIONS directory
        while ((entry = readdir(auctionsDirPtr)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char* AID = entry->d_name;
                // Check if the auction has ended
                char state = hasAuctionEnded(atoi(AID)) ? '0' : '1'; //TODO: REMOVE?  (myauctions, mybids, list) vêm tanto auctions ativos como inativos
                // Append AID and state to the string
                snprintf(auctionString, sizeof(auctionString), " %s %c", AID, state);
                strncat(response, auctionString, MAX_BUFFER_SIZE - strlen(response) - 1);
                auctionCount++;
            }
        }
        closedir(auctionsDirPtr);
    }

    // Check if there are ongoing auctions for the user
    if (auctionCount > 0) {
        // At least one ongoing auction
        strcat(response, "\n");
    } else {
        // No ongoing auctions for the user
        snprintf(response, MAX_BUFFER_SIZE, "LST NOK\n");
    }
}

void handleShowAssetRequest(char* request, char* response, int verbose) {
    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* AID_str = strtok(request, " ");
    int AID = atoi(AID_str);

    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);

    char startFilePath[100];
    snprintf(startFilePath, sizeof(startFilePath), "%s/START.txt", auctionDir);

    FILE* startFile = fopen(startFilePath, "r");
    // Check if the START file exists
    if (startFile != NULL) {
        // Read information from the START file
        char UID[UID_SIZE], name[MAX_FILENAME_SIZE], Fname[MAX_FILENAME_SIZE];
        int start_value, timeactive;
        fscanf(startFile, "%s %s %s %d %d", UID, name, Fname, &start_value, &timeactive);
        fclose(startFile);

        // Check if the asset file exists
        char assetFilePath[100];
        snprintf(assetFilePath, sizeof(assetFilePath), "%s/%s", auctionDir, Fname);

        FILE* assetFile = fopen(assetFilePath, "r");
        if (assetFile != NULL) {
            // Read file contents
            fseek(assetFile, 0, SEEK_END);
            long fileSize = ftell(assetFile);
            fseek(assetFile, 0, SEEK_SET);

            char* fileContents = (char*)malloc(fileSize + 1);
            fread(fileContents, 1, fileSize, assetFile);
            fclose(assetFile);

            // Null-terminate the file contents
            fileContents[fileSize] = '\0';//TODO: Is this needed? wont it overwrite the last character

            // Prepare response
            snprintf(response, MAX_BUFFER_SIZE, "RSA OK %s %ld %s\n", Fname, fileSize, fileContents);

            // Free allocated memory
            free(fileContents);
        } else {
            // Asset file not found
            snprintf(response, MAX_BUFFER_SIZE, "RSA NOK\n");
        }
    } else {
        // START file not found
        snprintf(response, MAX_BUFFER_SIZE, "RSA NOK\n");
    }
}

void handleBidRequest(char* request, char* response, int verbose) {//TODO falta checkar o minimum starting bid, maybe no new bid
    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");
    char* value_str = strtok(NULL, " ");

    int AID = atoi(AID_str);
    int value = atoi(value_str);

    // Check if UID and password are valid
    int validation = validateUser(UID, password); //TODO: check if user is already logged in first?

    if (validation == VALID_USER) {
        // Check if the auction exists and if the auction is ongoing 
        if (auctionExists(AID) && !auctionAlreadyEnded(AID)) {
            // Check if the user is trying to bid in an auction hosted by himself
            if (!auctionOwnedByUser(AID, UID)) {
                // Create a new bid
                int result = newBid(AID, value);

                if (result == 0) {
                    // Bid accepted
                    // Writes bid in user BIDDED directory
                    //TODO: checkar o valor do createFile
                    char bidDir[50];
                    snprintf(bidDir, sizeof(bidDir), "AS/USERS/%s/BIDDED", UID);
                    char bidFilename[50];
                    snprintf(bidFilename, sizeof(bidFilename), "%s.txt", AID_str);
                    createFile(bidDir, bidFilename);
                    strcpy(response, "RBD ACC\n");
                    return;
                } else if (result == -2) {
                    // Bid refused because a larger bid has already been placed
                    strcpy(response, "RBD REF\n");
                    return;
                } else {
                    // Error handling the new bid
                    strcpy(response, "RBD NOK\n");
                    return;
                }
            } else {
                // Illegal bid in an auction hosted by the user
                strcpy(response, "RBD ILG\n");
            }
        } else {
            // Auction does not exist
            strcpy(response, "RBD NOK\n");
        }
    } else if (validation == USER_NOT_EXIST || validation == INVALID_PASSWORD) {//TODO:Notspecified invalid password
        strcpy(response, "RBD NLG\n");
    } else {
        // Unexpected error
        strcpy(response, "RBD NOK\n");
    }
}

void handleShowRecordRequest(char* request, char* response, int verbose) { //TODO: not very well chcked came from chatGPT, não sei como vai funcionar mandar a a mensagems e o protocolo tem \n e é suposto acabar em \n
    if (verbose) {
        printf("Request received: %s\n", request);
    }
    
    char* AID_str = strtok(request, " ");
    int AID = atoi(AID_str);

    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "AS/AUCTIONS/%d", AID);
    
    char startFilePath[100];
    snprintf(startFilePath, sizeof(startFilePath), "%s/START.txt", auctionDir);

    // Check if the START file exists
    if (fileExists(startFilePath)) {
        // Read information from the START file
        char host_UID[UID_SIZE], auction_name[MAX_FILENAME_SIZE], asset_fname[MAX_FILENAME_SIZE];
        int start_value, timeactive;
        FILE* startFile = fopen(startFilePath, "r");
        fscanf(startFile, "%s %s %s %d %d", host_UID, auction_name, asset_fname, &start_value, &timeactive);
        fclose(startFile);

        // Prepare the initial response
        char currentDateTime[20];
        getCurrentDateTime(currentDateTime);
        snprintf(response, MAX_BUFFER_SIZE, "RRC OK %s %s %s %d %s %d", host_UID, auction_name, asset_fname, start_value, currentDateTime, timeactive);

        // Check if the auction has received bids
        char bidsDir[100];
        snprintf(bidsDir, sizeof(bidsDir), "%s/BIDS", auctionDir);

        DIR* bidsDirPtr = opendir(bidsDir);
        if (bidsDirPtr != NULL) {
            struct dirent* entry;
            while ((entry = readdir(bidsDirPtr)) != NULL) {
                if (entry->d_type == DT_REG) {  // Regular file
                    int bid_value = atoi(entry->d_name);
                    char bidFilePath[400];
                    snprintf(bidFilePath, sizeof(bidFilePath), "%s/%s", bidsDir, entry->d_name);

                    FILE* bidFile = fopen(bidFilePath, "r");
                    if (bidFile != NULL) {
                        char bidder_UID[UID_SIZE];
                        char bid_date_time[20];
                        int bid_sec_time;
                        fscanf(bidFile, "%s %d %s %d", bidder_UID, &bid_value, bid_date_time, &bid_sec_time);
                        fclose(bidFile);

                        // Append bid information to the response
                        snprintf(response + strlen(response), MAX_BUFFER_SIZE - strlen(response), "\nB %s %d %s %d", bidder_UID, bid_value, bid_date_time, bid_sec_time);// TODO: como vamos lidar com estes \n ???
                    }
                }
            }
            closedir(bidsDirPtr);
        }

        // Check if the auction is already closed
        char endFilePath[100];
        snprintf(endFilePath, sizeof(endFilePath), "%s/END.txt", auctionDir);

        if (fileExists(endFilePath)) {
            char end_date_time[20];
            int end_sec_time;
            FILE* endFile = fopen(endFilePath, "r");
            if (endFile != NULL) {
                fscanf(endFile, "%s %d", end_date_time, &end_sec_time);
                fclose(endFile);
                // Append closing information to the response
                snprintf(response + strlen(response), MAX_BUFFER_SIZE - strlen(response), "\nE %s %d", end_date_time, end_sec_time);
            }
        }
        // Append \n to the response
        sprintf(response + strlen(response), "\n");
    } else {
        // Auction not found
        snprintf(response, MAX_BUFFER_SIZE, "RRC NOK");
    }
}

void handleLogoutRequest(char* request, char* response, int verbose) {
    if (verbose) {
        printf("Request received: %s\n", request);
    }

    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password); //TODO: check if user is already logged in first?
    if (validation == VALID_USER) {
        // User is logged out
        char userDir[50];
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
        char filename[50];
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);

        // Delete the login file
        if(removeFile(filePath) == -1){
            snprintf(response, MAX_BUFFER_SIZE, "ERR\n");
            return;
        }

        strcpy(response, "RLO OK\n");
    } else if (validation == INVALID_PASSWORD) {// or if was not logged in
        strcpy(response, "RLO NOK\n");
    } else if (validation == USER_NOT_EXIST) {
        strcpy(response, "RLO UNR\n");
    }
}

void handleUnregisterRequest(char* request, char* response, int verbose) {
    if (verbose) {
        printf("Request received: %s\n", request);
    }
    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    // Validate user existence and password
    int validation = validateUser(UID, password); //TODO: check if user is already logged in first?

    if (validation == VALID_USER) {
        // User exists and password is correct, unregister the user
        // Delete password file
        char userDir[50];
        snprintf(userDir, sizeof(userDir), "AS/USERS/%s", UID);
        char filename[50];
        snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
        char filePath[100];
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        removeFile(filePath);

        // Delete login file
        snprintf(filename, sizeof(filename), "%s_login.txt", UID);
        snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
        removeFile(filePath);

        strcpy(response, "RUR OK\n");
    } else if (validation == USER_NOT_EXIST) {
        // User is not registered
        strcpy(response, "RUR UNR\n");
    } else {
        // User exists but the password is invalid
        strcpy(response, "RUR NOK\n");
    }
}

