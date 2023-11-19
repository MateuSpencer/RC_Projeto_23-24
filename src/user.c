#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_FILENAME_SIZE 24
#define MAX_PASSWORD_SIZE 9
#define SERVER_PORT 58000


// Function declarations
int createUDPSocket();
int createTCPSocket();
void sendUDPMessage(int socket, const char* message, struct sockaddr_in serverAddr);
void receiveUDPMessage(int socket, char* buffer, struct sockaddr_in* serverAddr);
void sendTCPMessage(int socket, const char* message);
void receiveTCPMessage(int socket, char* buffer);

void login(char* UID, char* password, char* ASIP, int ASport);
void openAuction(char* UID, char* password, char* name, char* asset_fname, int start_value, int timeactive);
void closeAuction(char* UID, char* password, int AID);
void myAuctions(char* UID);
void myBids(char* UID);
void listAuctions();
void showAsset(char* UID, char* password, int AID);
void bid(char* UID, char* password, int AID, int value);
void showRecord(char* UID, char* password, int AID);
void logout(char* UID, char* password);
void unregister(char* UID, char* password);
void exitApplication();

int main(int argc, char *argv[]) {
    // Parse command-line arguments

    // Application loop
    while (1) {
        // Display menu and get user input

        // Process user input and call corresponding function
    }

    return 0;
}

// Comunication Functions
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

void sendUDPMessage(int socket, const char* message, struct sockaddr_in serverAddr) {
    ssize_t n = sendto(socket, message, strlen(message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (n == -1) {
        perror("UDP sendto failed");
        exit(EXIT_FAILURE);
    }
}

void receiveUDPMessage(int socket, char* buffer, struct sockaddr_in* serverAddr) {
    socklen_t addrlen = sizeof(*serverAddr);
    ssize_t n = recvfrom(socket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)serverAddr, &addrlen);
    if (n == -1) {
        perror("UDP recvfrom failed");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0'; // Null-terminate the received data
}

void sendTCPMessage(int socket, const char* message) {
    ssize_t n = write(socket, message, strlen(message));
    if (n == -1) {
        perror("TCP write failed");
        exit(EXIT_FAILURE);
    }
}

void receiveTCPMessage(int socket, char* buffer) {
    ssize_t n = read(socket, buffer, MAX_BUFFER_SIZE - 1);
    if (n == -1) {
        perror("TCP read failed");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0'; // Null-terminate the received data
}

// User Actions Functions
void login(char* UID, char* password, char* ASIP, int ASport) {
    // UDP communication for login
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(ASport);
    serverAddr.sin_addr.s_addr = inet_addr(ASIP);

    // Prepare login message
    char loginMessage[MAX_BUFFER_SIZE];
    snprintf(loginMessage, sizeof(loginMessage), "LIN %s %s", UID, password);

    // Send login message
    sendUDPMessage(udpSocket, loginMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("Login Result: %s\n", reply);

    close(udpSocket);
}

void openAuction(char* UID, char* password, char* name, char* asset_fname, int start_value, int timeactive) {
    // TCP communication for opening an auction
    int tcpSocket = createTCPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Connect to the server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("TCP connect failed");
        exit(EXIT_FAILURE);
    }

    // Prepare open auction message
    char openAuctionMessage[MAX_BUFFER_SIZE];
    snprintf(openAuctionMessage, sizeof(openAuctionMessage), "OPA %s %s %s %d %d", UID, password, name, start_value, timeactive);

    // Send open auction message
    sendTCPMessage(tcpSocket, openAuctionMessage);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveTCPMessage(tcpSocket, reply);

    // Process reply and display results
    printf("Open Auction Result: %s\n", reply);

    close(tcpSocket);
}

void closeAuction(char* UID, char* password, int AID) {
    // TCP communication for closing an auction
    int tcpSocket = createTCPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Connect to the server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("TCP connect failed");
        exit(EXIT_FAILURE);
    }

    // Prepare close auction message
    char closeAuctionMessage[MAX_BUFFER_SIZE];
    snprintf(closeAuctionMessage, sizeof(closeAuctionMessage), "CLS %s %s %d", UID, password, AID);

    // Send close auction message
    sendTCPMessage(tcpSocket, closeAuctionMessage);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveTCPMessage(tcpSocket, reply);

    // Process reply and display results
    printf("Close Auction Result: %s\n", reply);

    close(tcpSocket);
}

void myAuctions(char* UID) {
    // UDP communication for listing auctions started by the user
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare my auctions message
    char myAuctionsMessage[MAX_BUFFER_SIZE];
    snprintf(myAuctionsMessage, sizeof(myAuctionsMessage), "LMA %s", UID);

    // Send my auctions message
    sendUDPMessage(udpSocket, myAuctionsMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("My Auctions Result: %s\n", reply);

    close(udpSocket);
}

void myBids(char* UID) {
    // UDP communication for listing auctions with bids by the user
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare my bids message
    char myBidsMessage[MAX_BUFFER_SIZE];
    snprintf(myBidsMessage, sizeof(myBidsMessage), "LMB %s", UID);

    // Send my bids message
    sendUDPMessage(udpSocket, myBidsMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("My Bids Result: %s\n", reply);

    close(udpSocket);
}

void listAuctions() {
    // UDP communication for listing all auctions
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare list auctions message
    char listAuctionsMessage[MAX_BUFFER_SIZE];
    snprintf(listAuctionsMessage, sizeof(listAuctionsMessage), "LST");

    // Send list auctions message
    sendUDPMessage(udpSocket, listAuctionsMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("List Auctions Result: %s\n", reply);

    close(udpSocket);
}

void showAsset(char* UID, char* password, int AID) {
    // TCP communication for showing the asset image
    int tcpSocket = createTCPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Connect to the server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("TCP connect failed");
        exit(EXIT_FAILURE);
    }

    // Prepare show asset message
    char showAssetMessage[MAX_BUFFER_SIZE];
    snprintf(showAssetMessage, sizeof(showAssetMessage), "SAS %d", AID);

    // Send show asset message
    sendTCPMessage(tcpSocket, showAssetMessage);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveTCPMessage(tcpSocket, reply);

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

    close(tcpSocket);
}

void bid(char* UID, char* password, int AID, int value) {
    // TCP communication for placing a bid
    int tcpSocket = createTCPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Connect to the server
    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("TCP connect failed");
        exit(EXIT_FAILURE);
    }

    // Prepare bid message
    char bidMessage[MAX_BUFFER_SIZE];
    snprintf(bidMessage, sizeof(bidMessage), "BID %s %s %d %d", UID, password, AID, value);

    // Send bid message
    sendTCPMessage(tcpSocket, bidMessage);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveTCPMessage(tcpSocket, reply);

    // Process reply and display results
    printf("Bid Result: %s\n", reply);

    close(tcpSocket);
}

void showRecord(char* UID, char* password, int AID) {
    // UDP communication for showing the auction record
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare show record message
    char showRecordMessage[MAX_BUFFER_SIZE];
    snprintf(showRecordMessage, sizeof(showRecordMessage), "SRC %s %s %d", UID, password, AID);

    // Send show record message
    sendUDPMessage(udpSocket, showRecordMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("Show Record Result: %s\n", reply);

    close(udpSocket);
}

void logout(char* UID, char* password) {
    // UDP communication for logging out
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare logout message
    char logoutMessage[MAX_BUFFER_SIZE];
    snprintf(logoutMessage, sizeof(logoutMessage), "LOU %s %s", UID, password);

    // Send logout message
    sendUDPMessage(udpSocket, logoutMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("Logout Result: %s\n", reply);

    close(udpSocket);
}

void unregister(char* UID, char* password) {
    // UDP communication for unregistering the user
    int udpSocket = createUDPSocket();
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT); // Replace with the correct server port
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Use loopback address for localhost

    // Prepare unregister message
    char unregisterMessage[MAX_BUFFER_SIZE];
    snprintf(unregisterMessage, sizeof(unregisterMessage), "UNR %s %s", UID, password);

    // Send unregister message
    sendUDPMessage(udpSocket, unregisterMessage, serverAddr);

    // Receive and process the reply
    char reply[MAX_BUFFER_SIZE];
    receiveUDPMessage(udpSocket, reply, &serverAddr);

    // Process reply and display results
    printf("Unregister Result: %s\n", reply);

    close(udpSocket);
}

void exitApplication() {
    // Implementation
}
