#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

// #include "common.h"

#define MAX_BUFFER_SIZE 6000
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

int UDPMessage(const char* message, char* reply, char* ASPort, char* ASIP);
int TCPMessage(const char* message, char* reply, char* ASPort, char* ASIP, int size);

int login(char* UID, char* password, char* ASIP, char* ASPort);
void openAuction(char* UID, char* password, char* name, char* asset_fname, char* start_value, char* timeactive,char* ASIP, char* ASPort);
void closeAuction(char* UID, char* password, char* AID, char* ASIP, char* ASPort);
void myAuctions(char* UID, char* ASIP, char* ASPort);
void myBids(char* UID, char* ASIP, char* ASPort);
void listAuctions(char* ASIP, char* ASPort);
void showAsset(char* AID, char* ASIP, char* ASPort);
void bid(char* UID, char* password, char* AID, char* value, char* ASIP, char* ASPort);
void showRecord(char* AID, char* ASIP, char* ASPort);
int logout(char* UID, char* password, char* ASIP, char* ASPort);
int unregister(char* UID, char* password, char* ASIP, char* ASPort);
char *readFile(const char *filename);

extern int errno;

int main(int argc, char *argv[]) {
    char ASIP[50] = "localhost";
    char ASport[6] = "58022";
    char UID[7];
    char password[9];
    int presentLoginCredentials = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            strncpy(ASIP, argv[i + 1], sizeof(ASIP) - 1);
            ASIP[sizeof(ASIP) - 1] = '\0';
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            strncpy(ASport, argv[i + 1], sizeof(ASport) - 1);
            ASport[sizeof(ASport) - 1] = '\0';
            i++;
        }
    }
    //TODO: Confirmar que pastas certas estão criadas?

    // Application loop
    while (1) {
        char token[50];

        printf("Digite um comando: ");
        scanf("%s", token);

        // Compare command and call the corresponding function
        if (strcmp(token, "login") == 0) {
            char UID_buffer[7];
            char password_buffer[9];
            scanf("%s %s", UID_buffer, password_buffer);
            int loginResponse = login(UID_buffer, password_buffer, ASIP, ASport);
            if(loginResponse == 0){//success
                strcpy(UID, UID_buffer);
                strcpy(password, password_buffer);
                presentLoginCredentials = 1;
            } else {
                presentLoginCredentials = 0;
            }

        } else if (strcmp(token, "open") == 0) {
            char name[100];
            char asset_fname[100];
            char start_value[100];
            char timeactive[100];
            scanf("%s %s %s %s", name, asset_fname, start_value, timeactive);
            openAuction(UID, password, name, asset_fname, start_value, timeactive, ASIP, ASport);

        } else if (strcmp(token, "close") == 0) {
            char AID[4];
            scanf("%s", AID);
            closeAuction(UID, password, AID, ASIP, ASport);

        } else if (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0) {
            myAuctions(UID, ASIP, ASport);

        } else if (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0) {
            myBids(UID, ASIP, ASport);

        } else if (strcmp(token, "list") == 0 || strcmp(token, "l") == 0) {
            listAuctions(ASIP, ASport);

        } else if (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0) {
            char AID[4];
            scanf("%s", AID);
            showAsset(AID, ASIP, ASport);

        } else if (strcmp(token, "bid") == 0 || strcmp(token, "b") == 0) {
            char AID[4];
            char value[MAX_BUFFER_SIZE];
            scanf("%s %s", AID, value);
            bid(UID, password, AID, value, ASIP , ASport);

        } else if (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0) {
            char AID[4];
            scanf("%s", AID);
            showRecord(AID, ASIP, ASport);

        } else if (strcmp(token, "logout") == 0){
            if(presentLoginCredentials == 0){
                printf("Please Login First\n");
            } else {
                presentLoginCredentials = logout(UID, password, ASIP, ASport);
                if(presentLoginCredentials == 0){
                    memset(&UID, 0, sizeof(UID));
                    memset(&password, 0, sizeof(password));
                }
            }

        } else if (strcmp(token, "unregister") == 0) {
            if(presentLoginCredentials == 0){
                printf("Please Login First\n");
            } else {
                presentLoginCredentials = unregister(UID, password, ASIP, ASport);
                if(presentLoginCredentials == 0){
                    memset(&UID, 0, sizeof(UID));
                    memset(&password, 0, sizeof(password));
                }
            }
        } else if (strcmp(token, "exit") == 0) {
            if(presentLoginCredentials == 0){
                break; // Exit the loop and end the program
            } else {
                printf("User is logged in. Please log out first.\n"); //TODO: what about if server has shut down?
            }

        } else {
            printf("Invalid command. Try again.\n");
        }
    }

    return 0;    
}

int UDPMessage(const char* message, char* reply, char* ASPort, char* ASIP) {
    int fd, errcode;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    fd = createUDPSocket();

    // Set a timeout on the socket
    struct timeval timeout;
    timeout.tv_sec = 1000000;  //TODO 5 seconds
    timeout.tv_usec = 0; // 0 microseconds
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    errcode = getaddrinfo(ASIP, ASPort, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(fd);
        return -1;
    }

    ssize_t received;
    if (sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen) != -1) {
        received = recvfrom(fd, reply, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
        if (received > 0) {
            char* newline_pos = strchr(reply, '\n');
            if (newline_pos != NULL) {
                newline_pos++;
                *newline_pos = '\0';
            }
            freeaddrinfo(res);
            close(fd);
            return received; // Return the number of bytes read
        } else if (received == -1 && errno == EWOULDBLOCK) {
            fprintf(stderr, "Timeout waiting for response\n");
        }
    }
    return -1;
}

int TCPMessage(const char* message, char* reply, char* ASPort, char* ASIP, int size) {
    int tcpSocket, errcode;
    ssize_t n;
    struct addrinfo hints, *res;

    tcpSocket = createTCPSocket();

    if (tcpSocket == -1) {
        return -1;
    }

    // Set a timeout on the socket
    struct timeval timeout;
    timeout.tv_sec = 1000000;  //TODO: 5 seconds
    timeout.tv_usec = 0; // 0 seconds
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("setsockopt");
        close(tcpSocket);
        return -1;
    }
    if (setsockopt(tcpSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
        perror("setsockopt");
        close(tcpSocket);
        return -1;
    }
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(ASIP, ASPort, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(tcpSocket);
        return -1;
    }

    n = connect(tcpSocket, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(tcpSocket);
        return -1;
    }

    size_t toWrite = strlen(message)+size;
    size_t written = 0;
    while (written < toWrite) {
        errno = 0;  // Reset errno before the call to write
        ssize_t n = write(tcpSocket, message + written, toWrite - written);
        if (n <= 0) {
            if (errno == EWOULDBLOCK) {
                fprintf(stderr, "Timeout occurred during write\n");
            } else {
                perror("write");
            }
            freeaddrinfo(res);
            close(tcpSocket);
            return -1;
        }
        written += (size_t)n;
    }

    size_t alreadyRead = 0;
    while (1) {
        errno = 0;  // Reset errno before the call to read
        ssize_t n = read(tcpSocket, reply + alreadyRead, 1 * sizeof(char));
        if (n <= 0) {
            if (errno == EWOULDBLOCK) {
                fprintf(stderr, "Timeout occurred during read\n");
            } else {
                perror("read");
            }
            freeaddrinfo(res);
            close(tcpSocket);
            return -1;
        }
        alreadyRead += (size_t)n;

        if (reply[alreadyRead - 1] == '\n') {
            break;
        } else if (alreadyRead >= MAX_BUFFER_SIZE + 1) {
            perror("read");
            freeaddrinfo(res);
            close(tcpSocket);
            return -1;
        }
    }
    reply[alreadyRead - 1] = '\0';

    freeaddrinfo(res);
    close(tcpSocket);
    return (ssize_t)alreadyRead; // Return the number of bytes read
}

// User Actions Functions
int login(char* UID, char* password, char* ASIP, char* ASPort) {
    char loginMessage[MAX_BUFFER_SIZE];
    snprintf(loginMessage, sizeof(loginMessage), "LIN %s %s\n", UID, password);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    int n = UDPMessage(loginMessage, reply, ASPort, ASIP);
    if (n == -1){
        printf("Error sending message\n");
        return -1;
    }

    // Process reply and display results
    if(strcmp(reply, "RLI OK\n") == 0){
        printf("Login Result: Successful!\n");
        return 0;
    } else if (strcmp(reply, "RLI NOK\n") == 0){
        printf("Login Result: Unsuccessful :(\n");
        return 1;
    } else if(strcmp(reply, "RLI REG\n") == 0){
        printf("Login Result: User Registered!\n");
        return 0;
    } else {
        printf("Unexpected reply: %s\n", reply);
        return 1;
    }
}

void openAuction(char* UID, char* password, char* name, char* asset_fname, char* start_value, char* timeactive, char* ASIP, char* ASPort) {
    //char openAuctionMessage[MAX_BUFFER_SIZE];

    // Open the file containing the asset
    FILE* assetFile = fopen(asset_fname, "rb");
    if (!assetFile) {
        printf("Error opening asset file: %s\n", asset_fname);
        return;
    }

    // Get the file size
    fseek(assetFile, 0, SEEK_END);
    long int fsize = ftell(assetFile);
    fseek(assetFile, 0, SEEK_SET);
    fclose(assetFile);

    char openAuctionMessage[MAX_BUFFER_SIZE+fsize];
    
    char *fileData = readFile(asset_fname);
    if (fileData == NULL) {
        printf("Error reading asset file: %s\n", asset_fname);
        return;
    }

    snprintf(openAuctionMessage, sizeof(openAuctionMessage), "OPA %s %s %s %s %s %s %ld %s\n", UID, password, name, start_value, timeactive, asset_fname, fsize, fileData);

    // Send open auction message
    char reply[MAX_BUFFER_SIZE];
    TCPMessage(openAuctionMessage, reply, ASPort, ASIP, fsize);

    // Process reply and display results
    if (strncmp(reply, "ROA OK", 6) == 0) {
        // Auction opened successfully
        int AID;
        sscanf(reply + 7, "%d", &AID);
        printf("Auction opened successfully! Auction ID: %d\n", AID);
    } else if (strncmp(reply, "ROA NOK", 7) == 0) {
        printf("Error opening auction.\n");
    } else if (strncmp(reply, "ROA NLG", 7) == 0) {
        printf("Error: User not logged in.\n");
    }else {
        printf("Unexpected reply: %s\n", reply);
    }
}

// Function to read the content of a file into a variable
char *readFile(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (!buffer) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, fileSize, file) != fileSize) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[fileSize] = '\0';

    fclose(file);
    return buffer;
}

void closeAuction(char* UID, char* password, char* AID, char* ASIP, char* ASPort) {
    // Prepare close auction message
    char closeAuctionMessage[MAX_BUFFER_SIZE];
    snprintf(closeAuctionMessage, sizeof(closeAuctionMessage), "CLS %s %s %s\n", UID, password, AID);

    // Send close auction message
    char reply[MAX_BUFFER_SIZE];
    TCPMessage(closeAuctionMessage, reply, ASPort, ASIP, 0);

    // Process reply and display results
    if (strncmp(reply, "RCL OK", 6) == 0) {
        printf("Auction closed successfully!\n");
    } else if (strncmp(reply, "RCL NLG", 7) == 0) {
        printf("Error: User not logged in.\n");
    } else if (strncmp(reply, "RCL EAU", 7) == 0) {
        printf("Error: Auction %s does not exist.\n", AID);
    } else if (strncmp(reply, "RCL EOW", 7) == 0) {
        printf("Error: Auction %s is not owned by user %s.\n", AID, UID);
    } else if (strncmp(reply, "RCL END", 7) == 0) {
        printf("Error: Auction %s owned by user %s has already finished.\n", AID, UID);
    }
}

//function used on mb, ma and l to parse their input
char (*parsePairs(const char *reply, char pairs[1998][4]))[4] {
    // Remove newline character from input
    char *mutableReply = strdup(reply);
    mutableReply[strcspn(mutableReply, "\n")] = '\0';

    // Parse pairs of words
    int count = 0;
    char *token = strtok(mutableReply + 6, " ");
    while (token != NULL && count < 1998) {
        // Save the pair
        strncpy(pairs[count], token, 3);
        pairs[count][3] = '\0'; // Null-terminate the word

        // Move to the next token
        token = strtok(NULL, " ");

        // Increment the count
        count++;

        // Check if there is another token (the size should be 1)
        if (token != NULL && count < 1988) {
            // Save the second word in the pair
            strncpy(pairs[count], token, 1);
            pairs[count][1] = '\0';

            // Move to the next token
            token = strtok(NULL, " ");

            // Increment the count
            count++;
        }
    }

    free(mutableReply); // Liberar a memória alocada por strdup

    return pairs;
}

void myAuctions(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMA %s\n", UID);

    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if (strcmp(reply, "RMA NOK\n") == 0){
        printf("%s has no auctions\n", UID);
    } else if(strcmp(reply, "RMA NLG\n") == 0){
        printf("%s is not logged in.\n", UID);
    } else {
        char pairs[1998][4];
        char (*result)[4] = parsePairs(reply, pairs);

        printf("%s's Auctions:\n", UID);
        for (int i = 0; i < 1998 && result[i][0] != '\0'; i += 2) {
            if(strcmp(result[i + 1],"1")==0){
                printf("Auction %s is active\n", result[i]);
            }else{
                printf("Auction %s is closed\n", result[i]);
            }
        }
    }
}

void myBids(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMB %s\n", UID);

    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if (strcmp(reply, "RMB NOK\n") == 0){
        printf("%s has no bids.\n", UID);
    } else if(strcmp(reply, "RMB NLG\n") == 0){
        printf("%s is not logged in.\n", UID);
    } else {
        char pairs[1998][4];
        char (*result)[4] = parsePairs(reply, pairs);

        printf("%s's Bids:\n", UID);
        for (int i = 0; i < 1998 && result[i][0] != '\0'; i += 2) {
            if(strcmp(result[i + 1],"1")==0){
                printf("You bidded on auction %s which is active\n", result[i]);
            }else{
                printf("You bidded on auction %s which is closed\n", result[i]);
            }
        }
    }
}

void listAuctions(char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LST\n");

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if(strcmp(reply, "RLS NOK\n") == 0){
        printf("No auctions have been started.\n");
    } else {
        char pairs[1998][4];
        char (*result)[4] = parsePairs(reply, pairs);
        printf("All Auctions:\n"); //TODO: better description
        for (int i = 0; i < 1998 && result[i][0] != '\0'; i += 2) {
            if(strcmp(result[i + 1],"1")==0){
                printf("Auction %s is active\n", result[i]);
            }else{
                printf("Auction %s is closed\n", result[i]);
            }
        }
    }
}

//TODO FIX - problema a recebner no TCPMessage
void showAsset(char* AID, char* ASIP, char* ASPort) {
    // Prepare show asset message
    char showAssetMessage[MAX_BUFFER_SIZE];
    snprintf(showAssetMessage, sizeof(showAssetMessage), "SAS %s", AID);

    char reply[MAX_BUFFER_SIZE];
    TCPMessage(showAssetMessage, reply, ASPort, ASIP, 0);

    // Process reply and display results
    if (strncmp(reply, "RSA OK", 6) == 0) {
        // Asset received successfully
        char* token = strtok(reply + 7, " ");
        char* fname = token;
        token = strtok(NULL, " ");
        char* fsize = token;
        token = strtok(NULL, " ");
        char* fdata = token;

        printf("Asset Received:\n");
        printf("Filename: %s\n", fname);
        printf("File Size: %s bytes\n", fsize);
        printf("File Data: %s\n", fdata);
    } else {
        printf("Show Asset Result: %s\n", reply);
    }
}

void bid(char* UID, char* password, char* AID, char* value, char* ASIP, char* ASPort) {
    char bidMessage[MAX_BUFFER_SIZE];
    snprintf(bidMessage, sizeof(bidMessage), "BID %s %s %s %s\n", UID, password, AID, value);

    char reply[MAX_BUFFER_SIZE];
    TCPMessage(bidMessage, reply, ASPort, ASIP, 0);

    // Process reply and display results
    if (strncmp(reply, "RBD ACC", 7) == 0) {
        printf("Bid accepted!\n");
    } else if (strncmp(reply, "RBD REF", 7) == 0) {
        printf("Bid refused: A larger bid has already been placed.\n");
    } else if (strncmp(reply, "RBD NOK", 7) == 0) {
        printf("Bid refused: Auction is not active.\n");
    } else if (strncmp(reply, "RBD NLG", 7) == 0) {
        printf("Error: User not logged in.\n");
    } else if (strncmp(reply, "RBD ILG", 7) == 0) {
        printf("Error: User cannot bid in an auction hosted by themselves.\n");
    }else {
        printf("Unexpected reply: %s\n", reply);
    }
}


void showRecord(char* AID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "SRC %s\n", AID);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if(strcmp(reply, "RRC NOK\n") == 0){
        printf("The auction %s does not exist\n", AID);
    } else {
        printf("Show record %s result: %s", AID, reply); //TODO: more human friendly
        // Delimiter characters
        const char *delimiters = "BE";

        char host_UID[7], auction_name[100], asset_fname[100], start_value[100], start_date[100], start_time[100], timeactive[100];
        char bidder_UID[7], bid_value[100], bid_date[100], bid_time[100], bid_sec_time[100];
        char end_date[100], end_time[100], end_sec_time[100];

        // Tokenize the string
        char *token = strtok(reply, delimiters);

        // Process and print tokens
        while (token != NULL) {
            printf("Segment: %s\n", token);
            if(sscanf(token, "RRC OK %s %s %s %s %s %s %s", host_UID, auction_name, asset_fname, start_value, start_date, start_time, timeactive) == 7){
                printf("AUCTION\n");
                printf("Host UID: %s\nAuction Name: %s\nAsset Name: %s\nStarting Value: %s\nStarting Date: %s\nStarting Time: %sTime Active: %s\n\n", host_UID, auction_name, asset_fname, start_value, start_date, start_time, timeactive);
            } else if(sscanf(token, "%s %s %s %s %s", bidder_UID, bid_value, bid_date, bid_time, bid_sec_time) == 5){
                printf("BID\n");
                printf("Bidder UID: %s\nBid Value: %s\nBid Date: %s\nBid Time: %s\nBid Time in Seconds: %s\n\n", host_UID, bid_value, bid_date, bid_time, bid_sec_time);
            } else if(sscanf(token, "%s %s %s", end_date, end_time, end_sec_time) == 3){
                printf("AUCTION HAS ENDED\n");
                printf("End Date: %s\nEnd Time: %s\nEnd Time in Seconds: %s\n", end_date, end_time, end_sec_time);
            }
            token = strtok(NULL, delimiters);
        }
    }
}

int logout(char* UID, char* password, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LOU %s %s\n", UID, password);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if(strcmp(reply, "RLO OK\n") == 0){
        printf("Logout Result: Successful!\n");
        return 0;
    } else if (strcmp(reply, "RLO NOK\n") == 0){
        printf("Logout Result: Unsuccessful, user is not logged in\n");
        return 0;
    } else if(strcmp(reply, "RLO UNR\n") == 0){
        printf("Logout Result: Unrecognized user\n");//TODO: please log in again & por return 0 e apagar credenciais do main?
        return 1;
    }

    return -1;
}

int unregister(char* UID, char* password, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "UNR %s %s\n", UID, password);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    UDPMessage(message, reply, ASPort, ASIP);

    // Process reply and display results
    if(strcmp(reply, "RUR OK\n") == 0){
        printf("Unregister Result: Successful!\n");
        return 0;
    } else if (strcmp(reply, "RUR NOK\n") == 0){
        printf("Unregister Result: Unsuccessful, user is not logged in\n");
        return 0;
    } else if(strcmp(reply, "RUR UNR\n") == 0){
        printf("Unregister Result: Unrecognized user\n");//TODO: please log in again & por return 0 e apagar credenciais do main?
        return 1;
    }
    return -1;
}
