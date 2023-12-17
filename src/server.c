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
#include <ctype.h>

#include "common.h"

#define VALID_USER 0
#define USER_NOT_EXIST 1
#define INVALID_PASSWORD 2
#define MISSING_PASSWORD 3

int handleUDPrequests(const char* Asport, int verbose);
int handleTCPrequests(const char* Asport, int verbose);

int createDirectory(const char *path);
int createFile(const char *directory, const char *filename);
int writeFile(const char *filePath, const char *content);
int removeFile(const char *filePath);
int delete_directory(const char *path);
int fileExists(const char* filePath);
void getCurrentDateTime(char* dateTime);

bool LoggedIn(const char* UID);
int validateUser(const char* UID, const char* password);
int isNumericAndLengthSix(const char *str);
int validateUserCredentialsFormating(char* UID, char* password);
int newAIDdirectory();
bool auctionExists(int AID);
bool auctionOwnedByUser(int AID, const char* UID);
int newBid(int AID, int newBidValue);
int auctionState(int AID);
int closeAuction(int AID);

void handleLoginRequest(char* request, char* response);
void handleOpenAuctionRequest(char* request, char* response);
void handleCloseAuctionRequest(char* request, char* response);
void handleMyAuctionsRequest(char* request, char* response);
void handleMyBidsRequest(char* request, char* response);
void handleListAuctionsRequest(char* response);
void handleShowAssetRequest(char* request, char* response);
void handleBidRequest(char* request, char* response);
void handleShowRecordRequest(char* request, char* response);
void handleLogoutRequest(char* request, char* response);
void handleUnregisterRequest(char* request, char* response);

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
    char Asport[6] = "58022";
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

    // SIGPIPE & SIGCHILD signal will be ignored
    struct sigaction act; 
    memset(&act,0,sizeof act);
    act.sa_handler=SIG_IGN;
    if(sigaction(SIGPIPE,&act,NULL)==-1)exit(EXIT_FAILURE);
    if(sigaction(SIGCHLD,&act,NULL)==-1)exit(EXIT_FAILURE);

    // Parse command-line arguments
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
        case 'p':
            // If port number is provided, update Asport
            snprintf(Asport, sizeof(Asport), "%s", optarg);
            break;
        case 'v':
            // If verbose flag is provided, set verbose to 1
            verbose = 1;
            break;
        default:
            // If invalid arguments are provided, print usage and exit
            fprintf(stderr, "Usage: %s [-p Asport] [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("To Terminate use CTRL+C or CTRL+Z\n");

    // Create the DB, USERS, and AUCTIONS directories if they don't exist
    char dir[50];
    snprintf(dir, sizeof(dir), "DB");
    if(createDirectory(dir) == -1){
        exit(EXIT_FAILURE);
    }
    snprintf(dir, sizeof(dir), "DB/AUCTIONS");
    if(createDirectory(dir) == -1){
        exit(EXIT_FAILURE);
    }
    snprintf(dir, sizeof(dir), "DB/USERS");
    if(createDirectory(dir) == -1){
        exit(EXIT_FAILURE);
    }
    
    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // If fork fails, print error and exit
        perror("Failed to fork into UDP and TCP listeners");
        exit(EXIT_FAILURE);
    } else if (pid != 0) {
        // Parent process
        handleTCPrequests(Asport, verbose);
    } else {
        // Child process
        if(handleUDPrequests(Asport, verbose) == -1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }

    // Wait for the child process to finish
    int status;
    waitpid(pid, &status, 0);
    return EXIT_SUCCESS;
}

int handleUDPrequests(const char* Asport, int verbose) {
    // Variable declarations
    int errcode;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char udpRequestBuffer[MAX_UDP_REQUEST_BUFFER_SIZE];
    char udpReplyBuffer[MAX_UDP_REPLY_BUFFER_SIZE];
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
    errcode = getaddrinfo(NULL, Asport, &hints, &res);
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
        printf("[UDP] Listening on port %s\n", Asport);
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
        n = recvfrom(udpSocket, udpRequestBuffer, sizeof(udpRequestBuffer), 0, (struct sockaddr *)&addr, &addrlen);
        if (n <= 0) {
            // If recvfrom fails, print error and continue to the next iteration
            perror("Error receiving UDP message");
            continue;
        }
        // Null-terminate the received data
        udpRequestBuffer[n] = '\0';

        if(verbose == 1){
            printf("[UDP] Received: %s\n", udpRequestBuffer);//TODO: a short description of the received requests (UID, type of request) and the IP and port originating those requests.
        }
        
        if (udpRequestBuffer[n - 1] != '\n' || strlen(udpRequestBuffer) < MIN_UDP_REQUEST_BUFFER_SIZE) {
            // If the last character is not a newline or if the request is smaller than the minimum, send an error response
            sprintf(udpReplyBuffer, "ERR\n"); 
        } else {
            // Process the received data
            char* action = strtok(udpRequestBuffer, " ");
            char* data = strtok(NULL, "\n");
            // Handle the action
            if (strcmp(action, "LIN") == 0) {
                handleLoginRequest(data, udpReplyBuffer);
            } else if (strcmp(action, "LMA") == 0) {
                handleMyAuctionsRequest(data, udpReplyBuffer);
            } else if (strcmp(action, "LMB") == 0) {
                handleMyBidsRequest(data, udpReplyBuffer);
            } else if (strcmp(action, "LST\n") == 0) {
                handleListAuctionsRequest(udpReplyBuffer);
            } else if (strcmp(action, "SRC") == 0) {
                handleShowRecordRequest(data, udpReplyBuffer);
            } else if (strcmp(action, "LOU") == 0) {
                handleLogoutRequest(data, udpReplyBuffer);
            } else if (strcmp(action, "UNR") == 0) {
                handleUnregisterRequest(data, udpReplyBuffer);
            } else {
                // If the action is unknown, send an error response
                sprintf(udpReplyBuffer, "ERR\n"); 
            }
        }
        // Send the response to the client
        n = sendto(udpSocket, udpReplyBuffer, sizeof(udpReplyBuffer), 0, (struct sockaddr *)&addr, addrlen);
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

int handleTCPrequests(const char* Asport, int verbose){
    // Variable declarations
    int errcode;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char tcpRequestBuffer[MAX_TCP_REQUEST_BUFFER_SIZE];
    char tcpReplyBuffer[MAX_TCP_REPLY_BUFFER_SIZE];

    // Create a TCP socket
    int tcpSocket = createTCPSocket();
    if (tcpSocket == -1) {
        return -1;
    }

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
    errcode = getaddrinfo(NULL, Asport, &hints, &res);
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
    if (listen(tcpSocket, MAX_TCP_CONNECTIONS) == -1) {
        // If listen fails, print error and exit
        perror("Error listening on TCP socket");
        return -1;
    }

    // Set up the address length
    addrlen = sizeof(addr);

    if (verbose) {
        printf("[TCP] Listening on port %s\n", Asport);
    }

    // Main loop to handle incoming connections
    while (!terminationRequested) {
        fd_set readSet;
        struct timeval timeout;
        FD_ZERO(&readSet);
        FD_SET(tcpSocket, &readSet);

        // Set a timeout for select to periodically check for termination
        timeout.tv_sec = 1;
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
            // Parent process
            close(sessionFd);
            continue;
            
        } else {
            // Child Process
            
            // Close the listening socket
            close(tcpSocket);
            // Read data from the client
            size_t alreadyRead = 0;
            ssize_t n;
            while (alreadyRead < MAX_TCP_REQUEST_BUFFER_SIZE && (n = read(sessionFd, tcpRequestBuffer + alreadyRead, sizeof(char))) > 0) {
                alreadyRead += (size_t)n;
                if (tcpRequestBuffer[alreadyRead - 1] == '\n') {
                    break;
                }
            }
            // Null-terminate the received data
            tcpRequestBuffer[alreadyRead] = '\0';

            if(verbose == 1){
                printf("[TCP] Received: %s\n", tcpRequestBuffer);//TODO: a short description of the received requests (UID, type of request) and the IP and port originating those requests.
            }
            if (tcpRequestBuffer[alreadyRead - 1] != '\n') {
                // If the last character is not a newline or if the request is smaller than the minimum, send an error response
                sprintf(tcpReplyBuffer, "ERR\n"); 
            } else {
                // Process the received data
                char* action = strtok(tcpRequestBuffer, " ");
                char* data = strtok(NULL, "\n");
                // Handle the action
                if (strcmp(action, "OPA") == 0) {
                    handleOpenAuctionRequest(data, tcpReplyBuffer);
                } else if (strcmp(action, "CLS") == 0) {
                    handleCloseAuctionRequest(data, tcpReplyBuffer);
                } else if (strcmp(action, "SAS") == 0) {
                    handleShowAssetRequest(data, tcpReplyBuffer);
                } else if (strcmp(action, "BID") == 0) {
                    handleBidRequest(data, tcpReplyBuffer);
                } else {
                    // If the action is unknown, send an error response
                    sprintf(tcpReplyBuffer, "ERR\n"); 
                }
            }
            // Send the response to the client
            size_t toWrite = strlen(tcpReplyBuffer);
            size_t written = 0;
            while (written < toWrite) {
                ssize_t sent = write(sessionFd, tcpReplyBuffer + written, MIN(MAX_TCP_REPLY_BUFFER_SIZE, toWrite - written));
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

int delete_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];

    // Open the directory
    if ((dir = opendir(path)) == NULL) {
        perror("opendir");
        return -1;
    }

    // Traverse the directory
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip "." and ".." entries
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursive call to delete subdirectory
            if (delete_directory(full_path) == -1) {
                closedir(dir);
                return -1;
            }
        } else {
            // Delete file
            if (unlink(full_path) == -1) {
                perror("unlink");
                closedir(dir);
                return -1;
            }
        }
    }

    // Close the directory
    closedir(dir);

    // Remove the directory itself
    if (rmdir(path) == -1) {
        perror("rmdir");
        return -1;
    }

    return 0;
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

// Function to get the current date and time in the required format
void getCurrentDateTime(char* dateTime) {
    time_t t;
    struct tm* tmp;

    t = time(NULL);
    tmp = localtime(&t);

    strftime(dateTime, 20, "%Y-%m-%d %H:%M:%S", tmp);
}
/* --------------------- HELPER FUNCTIONS  -----------------------------------*/

// Helper function to check if the user is logged in
bool LoggedIn(const char* UID) {
    char loginFile[50];
    snprintf(loginFile, sizeof(loginFile), "DB/USERS/%s/%s_login.txt", UID, UID);
    // Check if the login file exists
    if (access(loginFile, F_OK) != -1) {
        return true;  // User is logged in
    } else {
        return false;  // User is not logged in
    }
}

// Helper function to validate user existence and password
int validateUser(const char* UID, const char* password) {
    char userDir[50];
    snprintf(userDir, sizeof(userDir), "DB/USERS/%s", UID);
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
                char storedPassword[PASSWORD_SIZE];
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

int isNumericAndLengthSix(const char *str) {
    // Check if all characters are digits
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;  // Not a digit
        }
    }
    if (strlen(str) == 6) {
        return 1;  // Length is 6
    }
    return 0;  // Length is not 6
}

int validateUserCredentialsFormating(char* UID, char* password) {
    if (isNumericAndLengthSix(UID) && strlen(password) == 8){
        return 0;
    }else {
        return -1;
    }
}

// Helper function to find the highest AID in the AUCTIONS directory and create a new directory for the next AID
int newAIDdirectory() {
    DIR *dir = opendir("DB/AUCTIONS");
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
        snprintf(auctionDir, sizeof(auctionDir), "DB/AUCTIONS/%03d", nextAID); // MAX_AID_SIZE
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
    snprintf(auctionDir, sizeof(auctionDir), "DB/AUCTIONS/%03d", AID); // MAX_AID_SIZE

    struct stat st;
    return stat(auctionDir, &st) == 0 && S_ISDIR(st.st_mode);
}

// Helper function to check if an auction with the given AID is owned by the specified UID
bool auctionOwnedByUser(int AID, const char* UID) {
    char hostedDir[50];
    snprintf(hostedDir, sizeof(hostedDir), "DB/USERS/%s/HOSTED", UID);

    char auctionFile[100];
    snprintf(auctionFile, sizeof(auctionFile), "%s/%03d.txt", hostedDir, AID); // MAX_AID_SIZE

    return access(auctionFile, F_OK) != -1;
}

// Helper function to create a new bid if possible
int newBid(int AID, int newBidValue) {
    char bidsDir[50];
    snprintf(bidsDir, sizeof(bidsDir), "DB/AUCTIONS/%03d/BIDS", AID); // MAX_AID_SIZE

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
            snprintf(bidFilePath, sizeof(bidFilePath), "DB/AUCTIONS/%03d/BIDS", AID); // MAX_AID_SIZE
            char bidFilename[50];
            snprintf(bidFilename, sizeof(bidFilename), "%06d.txt", newBidValue); // MAX_BID_SIZE
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

// Helper function to check auction state and create END file if necessary
int auctionState(int AID) {
    char endFilePath[100];
    snprintf(endFilePath, sizeof(endFilePath), "DB/AUCTIONS/%03d/END.txt", AID); // MAX_AID_SIZE
    if(access(endFilePath, F_OK) != -1){
        return 0; //END file exists
    }else{
        //Check if the total time of the auction has passed
        char startFilePath[100];
        snprintf(startFilePath, sizeof(startFilePath), "DB/AUCTIONS/%03d/START.txt", AID); // MAX_AID_SIZE
        FILE *startFile = fopen(startFilePath, "r");
        if (startFile != NULL) {
            int timeactive;
            time_t creationTime;
            if (fscanf(startFile, "%*s %*s %*s %*d %d %*s %*s %ld", &timeactive, &creationTime) == 2) {
                fclose(startFile);
                time_t now = time(NULL);
                if (creationTime + timeactive < now) {
                    if (closeAuction(AID)  == -1) {
                        return -1;
                    }
                    return 0;
                }
            }else{
                fclose(startFile);
                return -1;
            }
        }else{
            return -1;
        }
    }
    return 1;
}

int closeAuction(int AID) {
    char endFilePath[100];
    snprintf(endFilePath, sizeof(endFilePath), "DB/AUCTIONS/%03d", AID); // MAX_AID_SIZE
    if(createFile(endFilePath, "END.txt") == -1){
        return -1;
    }
    //obtain the starting time from the START file
    char startFilePath[100];
    snprintf(startFilePath, sizeof(startFilePath), "DB/AUCTIONS/%03d/START.txt", AID); // MAX_AID_SIZE
    FILE *startFile = fopen(startFilePath, "r");
    if (startFile != NULL) {
        int timeactive;
        time_t creationTime;
        if (fscanf(startFile, "%*s %*s %*s %*d %d %*s %*s %ld", &timeactive, &creationTime) == 2) {
            fclose(startFile);
            time_t now = time(NULL);
            int elapsedTime = (int)difftime(now, creationTime);
            if (elapsedTime < timeactive) {
                timeactive = elapsedTime;
            }
            // Get the current date and time
            char currentDateTime[20];
            getCurrentDateTime(currentDateTime);
            char content[50];
            snprintf(content, sizeof(content), "%s %d", currentDateTime, timeactive);
            snprintf(endFilePath, sizeof(endFilePath), "DB/AUCTIONS/%03d/END.txt", AID); // MAX_AID_SIZE
            if (writeFile(endFilePath, content) == -1) {
                return -1;
            }
            return 0;
        }else{
            fclose(startFile);
            return -1;
        }
    }else{
        return -1;
    }
    return -1;
}

/* --------------------- HANDLER FUNCTIONS  -----------------------------------*/

void handleLoginRequest(char* request, char* response) {
    char userDir[50];
    char filename[50];
    char filePath[100];

    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
        return;
    }
    if (LoggedIn(UID)) {
        //User is already Logged in
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI OK\n");
        return;
    }else{
        // Validate user existence and password
        int validation = validateUser(UID, password);

        if (validation == VALID_USER || validation == MISSING_PASSWORD) {
            // User Exists
            snprintf(userDir, sizeof(userDir), "DB/USERS/%s", UID);
            snprintf(filename, sizeof(filename), "%s_login.txt", UID);
            
            if(createFile(userDir, filename) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI OK\n");

            if(validation == MISSING_PASSWORD){
                //if user was registered but had no password, assign new password
                snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
                if(createFile(userDir, filename) == -1){
                    snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                    return;
                }
                snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
                if(writeFile(filePath, password) == -1){
                    snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                    return;
                }
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI REG\n");
            }
            return;
        } else if (validation == USER_NOT_EXIST) {
            // User is not registered, register and log in the user

            // Construct user directory path
            snprintf(userDir, sizeof(userDir), "DB/USERS/%s", UID);
            // Create user directory
            if(createDirectory(userDir) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }

            // Create password file
            snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
            if(createFile(userDir, filename) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            // Create login file
            snprintf(filename, sizeof(filename), "%s_login.txt", UID);
            if(createFile(userDir, filename) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            
            // Store the password in the password file
            snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            if(writeFile(filePath, password) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            
            // Create HOSTED and BIDDED directories
            char userDirCopy[50];
            strcpy(userDirCopy, userDir);
            if(createDirectory(strcat(userDir, "/HOSTED")) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            if(createDirectory(strcat(userDirCopy, "/BIDDED")) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
                return;
            }
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI REG\n");
            return;
        } else if(validation == INVALID_PASSWORD) {
            // User exists but the password is invalid
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
            return;
        }else{
            // Unexpected error
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLI NOK\n");
            return;
        }
    }
}

void handleOpenAuctionRequest(char *request, char *response) {
    char *UID = strtok(request, " ");
    char *password = strtok(NULL, " ");
    char *name = strtok(NULL, " ");
    char *start_value_str = strtok(NULL, " ");
    char *timeactive_str = strtok(NULL, " ");
    char *Fname = strtok(NULL, " ");
    char *Fsize_str = strtok(NULL, " ");
    (void)Fsize_str; // unused
    char *Fdata = strtok(NULL, "\n");

    (void)Fsize_str; // unused
    int start_value = atoi(start_value_str);
    int timeactive = atoi(timeactive_str);

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "ROA NOK\n");
        return;
    }
    
    if (LoggedIn(UID)) {
        int AID = newAIDdirectory();
        if (AID != -1) {
            char AIDdirectory[100];
            snprintf(AIDdirectory, sizeof(AIDdirectory), "DB/AUCTIONS/%03d", AID); // MAX_AID_SIZE
            // Create START file
            char startFilePath[100];
            snprintf(startFilePath, sizeof(startFilePath), "DB/AUCTIONS/%03d/START.txt", AID); // MAX_AID_SIZE
            FILE *startFile = fopen(startFilePath, "w");
            if (startFile != NULL) {
                // Get the current time
                time_t now = time(NULL);
                // Format the time as a string
                char currentDateTime[20];
                getCurrentDateTime(currentDateTime);
                // Write the data to the file
                fprintf(startFile, "%s %s %s %d %d %s %ld", UID, name, Fname, start_value, timeactive, currentDateTime, (long)now);
                fclose(startFile);
            } else {
                perror("Error creating START file");
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "NOK\n");
                delete_directory(AIDdirectory);
                return;
            }
            // Create asset file
            char assetFilePath[100];
            snprintf(assetFilePath, sizeof(assetFilePath), "DB/AUCTIONS/%03d/%s", AID, Fname); // MAX_AID_SIZE
            if(writeFile(assetFilePath, Fdata) == -1){
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "NOK\n");
                delete_directory(AIDdirectory);
                return;
            }

            // Create BIDS directory
            char bidsDir[50];
            snprintf(bidsDir, sizeof(bidsDir), "DB/AUCTIONS/%03d/BIDS", AID); // MAX_AID_SIZE
            if(createDirectory(bidsDir) == -1){
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "NOK\n");
                delete_directory(AIDdirectory);
                return;
            }

            // Create an empty file in the UID's HOSTED directory with the AID as the filename
            char hostedDir[50];
            snprintf(hostedDir, sizeof(hostedDir), "DB/USERS/%s/HOSTED", UID);
            if(createDirectory(hostedDir) == -1){
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "NOK\n");
                delete_directory(AIDdirectory);
                return;
            }

            char hostedFilename[100];
            snprintf(hostedFilename, sizeof(hostedFilename), "%03d.txt", AID); // MAX_AID_SIZE
            
            if(createFile(hostedDir, hostedFilename) == -1){
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "NOK\n");
                delete_directory(AIDdirectory);
                return;
            }

            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "ROA OK %03d\n", AID); // MAX_AID_SIZE
            return;
        } else {
            // Error creating new AID directory
            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "ROA NOK\n");
            return;
        }
    }else{
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "ROA NLG\n");
        return;
    }
}

void handleCloseAuctionRequest(char* request, char* response) {
    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");

    int AID = atoi(AID_str);

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL NOK\n");
        return;
    }
    
    if (LoggedIn(UID)) {
        // Check if the auction exists
        if (auctionExists(AID) == 1) {
            // Check if the auction is owned by the user
            if (auctionOwnedByUser(AID, UID)) {
                // Check if END.txt file is present
                int state = auctionState(AID);
                if (state == 1){
                    // Close the auction by creating the END file
                    if(closeAuction(AID) == -1){
                        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL NOK\n");
                        return;
                    }
                    snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL OK\n");
                    return;
                } else if(state == 0){
                    snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL END\n"); // Auction has already ended
                }else{
                    // Unexpected error
                    snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL NOK\n");
                    return;
                }
            } else {
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL EOW\n"); // Auction is not owned by the user
            }
        } else {
            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL EAU\n"); // Auction does not exist
        }
    }else{
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RCL NLG\n");
        return;
    }
}

void handleMyAuctionsRequest(char* request, char* response) {
    char auctionString[10];
    int auctionCount = 0;

    char* UID = strtok(request, " ");

    if (LoggedIn(UID)) {
        // Initialize response with "RMA OK"
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMA OK");

        char Dir[50];
        struct dirent* entry;

        for (int i = 0; i < 2; i++) {
            if (i == 0) {
                snprintf(Dir, sizeof(Dir), "DB/USERS/%s/HOSTED", UID);
            } else {
                snprintf(Dir, sizeof(Dir), "DB/USERS/%s/BIDDED", UID);
            }
            DIR* DirPtr = opendir(Dir);

            if (DirPtr != NULL) {
                // Loop through the directory
                while ((entry = readdir(DirPtr)) != NULL) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        char* AID_str = entry->d_name;
                        int AID = atoi(AID_str);
                        int state = auctionState(AID);
                        if(state == -1){
                            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMA NOK\n");
                            return;
                        }
                        // Append AID and state to the string
                        snprintf(auctionString, sizeof(auctionString), " %03d %d", AID, state); // MAX_AID_SIZE
                        strncat(response, auctionString, MAX_UDP_REPLY_BUFFER_SIZE - strlen(response) - 1);
                        auctionCount++;
                    }
                }
                closedir(DirPtr);
            }
        }
        // Check if there are ongoing auctions for the user
        if (auctionCount > 0) {
            // At least one ongoing auction
            strcat(response, "\n");
        } else {
            // No ongoing auctions for the user
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMA NOK\n");
        }
    }else{
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RMA NLG\n");
        return;
    }
}

void handleMyBidsRequest(char* request, char* response) {
    char bidString[10];
    int bidCount = 0;

    char* UID = strtok(request, " ");

    if (LoggedIn(UID)) {
        // Initialize response with "RMB OK"
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMB OK");

        char Dir[50];
        struct dirent* entry;

        snprintf(Dir, sizeof(Dir), "DB/USERS/%s/BIDDED", UID);
        // Open the BIDDED directory
        DIR* DirPtr = opendir(Dir);
        if (DirPtr != NULL) {
            // Loop through the BIDDED directory
            while ((entry = readdir(DirPtr)) != NULL) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    char* AID_str = entry->d_name;
                    int AID = atoi(AID_str);
                    int state = auctionState(AID);
                    if(state == -1){
                        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMB NOK\n");
                        return;
                    }
                    // Append AID and state to the string
                    snprintf(bidString, sizeof(bidString), " %03d %d", AID, state); // MAX_AID_SIZE
                    strncat(response, bidString, MAX_UDP_REPLY_BUFFER_SIZE - strlen(response) - 1);
                    bidCount++;
                }
            }
            closedir(DirPtr);
        }
        // Check if there are bids for the user
        if (bidCount > 0) {
            // At least one bid
            strcat(response, "\n");
        } else {
        // No bids
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMB NOK\n");
        }
    } else {
        // User is not logged in
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RMB NLG\n");
    }
}

void handleListAuctionsRequest(char* response) {
    char auctionString[10];
    int auctionCount = 0;

    // Initialize response with "RLS OK"
    snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLS OK");
    
    // Open the AUCTIONS directory
    DIR* DirPtr = opendir("DB/AUCTIONS");

    if (DirPtr != NULL) {
        struct dirent* entry;
        // Loop through the AUCTIONS directory
        while ((entry = readdir(DirPtr)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char* AID_str = entry->d_name;
                int AID = atoi(AID_str);
                int state = auctionState(AID);
                if(state == -1){
                    snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLS NOK\n");
                    return;
                }
                // Append AID and state to the string
                snprintf(auctionString, sizeof(auctionString), " %03d %d", AID, state); // MAX_AID_SIZE
                strncat(response, auctionString, MAX_UDP_REPLY_BUFFER_SIZE - strlen(response) - 1);
                auctionCount++;
            }
        }
        closedir(DirPtr);
    }
    // Check if there are ongoing auctions
    if (auctionCount > 0) {
        // At least one ongoing auction
        strcat(response, "\n");
    } else {
        // No ongoing auctions
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLS NOK\n");
    }
}

void handleShowAssetRequest(char* request, char* response) {
    char* AID_str = strtok(request, " ");
    int AID = atoi(AID_str);

    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "DB/AUCTIONS/%03d", AID); // MAX_AID_SIZE

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
            fileContents[fileSize] = '\0';

            // Prepare response
            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RSA OK %s %ld %s\n", Fname, fileSize, fileContents);

            // Free allocated memory
            free(fileContents);
        } else {
            // Asset file not found
            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RSA NOK\n");
            return;
        }
    } else {
        // START file not found
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RSA NOK\n");
        return;
    }
}

void handleBidRequest(char* request, char* response) {
    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");
    char* AID_str = strtok(NULL, " ");
    char* value_str = strtok(NULL, " ");

    int AID = atoi(AID_str);
    int value = atoi(value_str);

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "ROA NOK\n");
        return;
    }

    if (LoggedIn(UID)) {
        // Check if the auction exists and if the auction is ongoing 
        if (auctionExists(AID) == 1 && auctionState(AID) == 1) {
            // Check if the user is trying to bid in an auction hosted by himself
            if (!auctionOwnedByUser(AID, UID)){
                //obtain the start_value from the START file
                char startFilePath[100];
                snprintf(startFilePath, sizeof(startFilePath), "DB/AUCTIONS/%03d/START.txt", AID); // MAX_AID_SIZE
                FILE *startFile = fopen(startFilePath, "r");
                if (startFile != NULL) {
                    int start_value;
                    if (fscanf(startFile, "%*s %*s %*s %d %*d %*s %*s %*d", &start_value) == 1) {
                        fclose(startFile);
                        // Check if bid is larger or equal than the starting value
                        if (value <= start_value) {
                            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD REF\n");
                            return;
                        }
                        // Create a new bid
                        int result = newBid(AID, value);
                        if (result == 0) {
                            // Bid accepted
                            // Writes bid contents
                            char bidFilePath[100];
                            snprintf(bidFilePath, sizeof(bidFilePath), "DB/AUCTIONS/%03d/BIDS/%06d.txt", AID, value); // MAX_BID_SIZE
                            // Get the current date and time
                            char currentDateTime[20];
                            getCurrentDateTime(currentDateTime);
                            char content[100];
                            char startFilePath[100];
                            int elapsedTime;
                            snprintf(startFilePath, sizeof(startFilePath), "DB/AUCTIONS/%03d/START.txt", AID); // MAX_AID_SIZE
                            FILE *startFile = fopen(startFilePath, "r");
                            if (startFile != NULL) {
                                time_t creationTime;
                                if (fscanf(startFile, "%*s %*s %*s %*d %*d %*s %*s %ld", &creationTime) == 1) {
                                    fclose(startFile);
                                    time_t now = time(NULL);
                                     elapsedTime = (int)difftime(now, creationTime);
                                }
                            }
                            snprintf(content, sizeof(content), "%s %d %s %d", UID, value, currentDateTime, elapsedTime);
                            if (writeFile(bidFilePath, content) == -1) {
                                // Error handling the new bid
                                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
                                removeFile(bidFilePath);
                                return;
                            }
                            // Writes bid in user BIDDED directory
                            char bidDir[50];
                            snprintf(bidDir, sizeof(bidDir), "DB/USERS/%s/BIDDED", UID);
                            char bidFilename[50];
                            snprintf(bidFilename, sizeof(bidFilename), "%03d.txt", AID); // MAX_AID_SIZE
                            if(createFile(bidDir, bidFilename) == -1){
                                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
                                return;
                            }
                            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD ACC\n");
                            return;
                        } else if (result == -2) {
                            // Bid refused because a larger bid has already been placed
                            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD REF\n");
                            return;
                        } else {
                            // Error handling the new bid
                            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
                            return;
                        }
                    }else{
                        fclose(startFile);
                        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
                        return;
                    }
                }else{
                    //error opening START file
                    snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
                    return;
                }
            } else {
                // Illegal bid in an auction hosted by the user
                snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD ILG\n");
                return;
            }
        } else {
            // Auction does not exist
            snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NOK\n");
            return;
        }
    } else {
        snprintf(response, MAX_TCP_REPLY_BUFFER_SIZE, "RBD NLG\n");
        return;
    }
}

void handleShowRecordRequest(char* request, char* response) {
    char* AID_str = strtok(request, " ");
    int AID = atoi(AID_str);

    char auctionDir[50];
    snprintf(auctionDir, sizeof(auctionDir), "DB/AUCTIONS/%03d", AID); // MAX_AID_SIZE
    
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
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RRC OK %s %s %s %d %s %d ", host_UID, auction_name, asset_fname, start_value, currentDateTime, timeactive);

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
                        char bid_date[20];
                        char bid_time[20];
                        int bid_sec_time;
                        fscanf(bidFile, "%s %d %s %s %d", bidder_UID, &bid_value, bid_date, bid_time, &bid_sec_time);
                        fclose(bidFile);

                        // Append bid information to the response
                        snprintf(response + strlen(response), MAX_UDP_REPLY_BUFFER_SIZE - strlen(response), "B %s %d %s %s %d ", bidder_UID, bid_value, bid_date, bid_time, bid_sec_time);
                    }
                }
            }
            closedir(bidsDirPtr);
        }
        int state = auctionState(AID);
        //if(state == 0){
        if(state != 1){
            char endFilePath[100];
            snprintf(endFilePath, sizeof(endFilePath), "%s/END.txt", auctionDir);
            if (fileExists(endFilePath)) {
                char end_date[20];
                char end_time[20];
                int end_sec_time;
                FILE* endFile = fopen(endFilePath, "r");
                if (endFile != NULL) {
                    fscanf(endFile, "%s %s %d", end_date, end_time, &end_sec_time);
                    fclose(endFile);
                    // Append closing information to the response
                    snprintf(response + strlen(response), MAX_UDP_REPLY_BUFFER_SIZE - strlen(response), "E %s %s %d", end_date, end_time, end_sec_time);
                }
            }
        }
        
        // Append \n to the response
        sprintf(response + strlen(response), "\n");
    } else {
        // Auction not found
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RRC NOK");
        return;
    }
}

void handleLogoutRequest(char* request, char* response) {
    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO NOK\n");
        return;
    }
    if (LoggedIn(UID)) {
        // Validate user existence and password
        int validation = validateUser(UID, password);
        if (validation == VALID_USER) {
            // User is logged out
            char userDir[50];
            snprintf(userDir, sizeof(userDir), "DB/USERS/%s", UID);
            char filename[50];
            snprintf(filename, sizeof(filename), "%s_login.txt", UID);
            char filePath[100];
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            // Delete the login file
            if(removeFile(filePath) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO NOK\n");
                return;
            }
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO OK\n");
            return;
        } else if (validation == INVALID_PASSWORD) {
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO NOK\n");//Not specified
            return;
        } else if (validation == USER_NOT_EXIST || validation == MISSING_PASSWORD) {
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO UNR\n");
            return;
        }else{
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO NOK\n");
            return;
        }
    }else{
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RLO NOK\n");
        return;
    }
}

void handleUnregisterRequest(char* request, char* response) {
    char* UID = strtok(request, " ");
    char* password = strtok(NULL, " ");

    if(validateUserCredentialsFormating(UID, password) == -1){
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
        return;
    }

    if (LoggedIn(UID)) {
        // Validate user existence and password
        int validation = validateUser(UID, password);
        if (validation == VALID_USER) {
            // User exists and password is correct, unregister the user
            // Delete password file
            char userDir[50];
            snprintf(userDir, sizeof(userDir), "DB/USERS/%s", UID);
            char filename[50];
            snprintf(filename, sizeof(filename), "%s_pass.txt", UID);
            char filePath[100];
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            if(removeFile(filePath)== -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
                return;
            }
            // Delete login file
            snprintf(filename, sizeof(filename), "%s_login.txt", UID);
            snprintf(filePath, sizeof(filePath), "%s/%s", userDir, filename);
            if(removeFile(filePath) == -1){
                snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
                return;
            }
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR OK\n");
            return;
        } else if (validation == USER_NOT_EXIST || validation == MISSING_PASSWORD) {
            // User is not registered
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR UNR\n");
            return;
        } else if(validation == INVALID_PASSWORD){
            // User exists but the password is invalid
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
            return;
        }else{
            snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
            return;
        }
    }else{
        snprintf(response, MAX_UDP_REPLY_BUFFER_SIZE, "RUR NOK\n");
        return;
    }
}


