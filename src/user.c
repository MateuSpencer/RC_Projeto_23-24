#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_FILENAME_SIZE 24
#define MAX_PASSWORD_SIZE 9
#define SERVER_PORT 58000


// Function declarations
int createUDPSocket();
int createTCPSocket();
void sendUDPMessage(const char* message, char* reply, char* ASPort, char* ASIP);
void sendTCPMessage(int socket, const char* message);
void receiveTCPMessage(int socket, char* buffer);

void login(char* UID, char* password, char* ASIP, char* ASport);
void openAuction(char* UID, char* password, char* name, char* asset_fname, int start_value, int timeactive,char* ASIP, char* ASport);
void closeAuction(char* UID, char* password, int AID, char* ASIP, char* ASport);
void myAuctions(char* UID, char* ASIP, char* ASPort);
void myBids(char* UID, char* ASIP, char* ASPort);
void listAuctions(char* ASIP, char* ASPort);
void showAsset(int AID, char* ASIP, char* ASport);
void bid(char* UID, char* password, int AID, int value, char* ASIP, char* ASport);
void showRecord(int AID, char* ASIP, char* ASPort);
void logout(char* UID, char* password, char* ASIP, char* ASPort);
void unregister(char* UID, char* password, char* ASIP, char* ASPort);
void exitApplication();

int main(int argc, char *argv[]) {
    char IP[] = "tejo.tecnico.ulisboa.pt"; 
    char Port[] = "58011";

    char *UID;
    char *password;

    // Assume the maximum length of a command is 50 characters
    char input[50];

    // Application loop
    while (1) {
        printf("Enter a command: ");
        fgets(input, sizeof(input), stdin);

        // Remove newline character from input
        input[strcspn(input, "\n")] = '\0';

        // Tokenize the input to extract command and arguments
        char *token = strtok(input, " ");
        if (token == NULL) {
            continue; // Empty input, ask again
        }

        // Compare command and call the corresponding function
        if (strcmp(token, "login") == 0) {
            UID = strtok(NULL, " ");
            password = strtok(NULL, " ");
            login(UID, password, IP, Port);

        } else if (strcmp(token, "open") == 0) {
            char *name = strtok(NULL, " ");
            char *asset_fname = strtok(NULL, " ");
            int start_value = atoi(strtok(NULL, " "));
            int timeactive = atoi(strtok(NULL, " "));
            openAuction(UID, password, name, asset_fname, start_value, timeactive, IP, Port);

        } else if (strcmp(token, "close") == 0) {
            int AID = atoi(strtok(NULL, " "));
            closeAuction(UID, password, AID, IP, Port);

        } else if (strcmp(token, "myauctions") == 0 || strcmp(token, "ma") == 0) {
            myAuctions(UID, IP, Port);

        } else if (strcmp(token, "mybids") == 0 || strcmp(token, "mb") == 0) {
            myBids(UID, IP, Port);

        } else if (strcmp(token, "list") == 0 || strcmp(token, "l") == 0) {
            listAuctions(IP, Port);

        } else if (strcmp(token, "show_asset") == 0 || strcmp(token, "sa") == 0) {
            int AID = atoi(strtok(NULL, " "));
            showAsset(AID, IP, Port);

        } else if (strcmp(token, "bid") == 0) {
            int AID = atoi(strtok(NULL, " "));
            int value = atoi(strtok(NULL, " "));
            bid(UID, password, AID, value, IP , Port);

        } else if (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0) {
            int AID = atoi(strtok(NULL, " "));
            showRecord(AID, IP, Port);

        } else if (strcmp(token, "logout") == 0) {
            logout(UID, password, IP, Port);

        } else if (strcmp(token, "unregister") == 0) {
            unregister(UID, password, IP, Port);

        } else if (strcmp(token, "exit") == 0) {
            exitApplication();
            break; // Exit the loop and end the program

        } else {
            printf("Invalid command. Try again.\n");
        }
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

void sendUDPMessage(const char* message, char* reply, char* ASPort, char* ASIP) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen; // Tamanho do endereço
    /*
    hints - Estrutura que contém informações sobre o tipo de conexão que será estabelecida.
            Podem-se considerar, literalmente, dicas para o sistema operacional sobre como
            deve ser feita a conexão, de forma a facilitar a aquisição ou preencher dados.

    res - Localização onde a função getaddrinfo() armazenará informações sobre o endereço.
    */
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128]; // buffer para onde serão escritos os dados recebidos do servidor

    /* Cria um socket UDP (SOCK_DGRAM) para IPv4 (AF_INET).
    É devolvido um descritor de ficheiro (fd) para onde se deve comunicar. */
    fd = createUDPSocket();

    /* Preenche a estrutura com 0s e depois atribui a informação já conhecida da ligação */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    /* Busca informação do host "localhost", na porta especificada,
    guardando a informação nas `hints` e na `res`. Caso o host seja um nome
    e não um endereço IP (como é o caso), efetua um DNS Lookup. */
    errcode = getaddrinfo(ASPort, ASIP, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }

    /* Envia para o `fd` (socket) a mensagem "Hello!\n" com o tamanho 7.
       Não são passadas flags (0), e é passado o endereço de destino.
       É apenas aqui criada a ligação ao servidor. */
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    /* Recebe 128 Bytes do servidor e guarda-os no buffer.
       As variáveis `addr` e `addrlen` não são usadas pois não foram inicializadas. */
    addrlen = sizeof(addr);
    n = recvfrom(fd, reply, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
    if (n == -1) {
        exit(1);
    }

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);
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
void login(char* UID, char* password, char* ASIP, char* ASPort) {
    
    char loginMessage[MAX_BUFFER_SIZE];
    snprintf(loginMessage, sizeof(loginMessage), "LIN %s %s\n", UID, password);

    printf("%s", loginMessage);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(loginMessage, reply, ASIP, ASPort);
    printf("%s", reply);
    // Process reply and display results
    if(strcmp(reply, "RLI OK\n") == 0){
        printf("Login Result: Successful!\n");
    } else if (strcmp(reply, "RLI NOK\n") == 0){
        printf("Login Result: Unsuccessful :(\n");
    } else if(strcmp(reply, "RLI REG\n") == 0){
        printf("Login Result: User Registered!\n");
    }
}

void openAuction(char* UID, char* password, char* name, char* asset_fname, int start_value, int timeactive,char* ASIP, char* ASport){
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

void closeAuction(char* UID, char* password, int AID, char* ASIP, char* ASport) {
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

void myAuctions(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMA %s\n", UID);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if (strcmp(reply, "RMA NOK\n") == 0){
        printf("%s has no auctions\n", UID);
    } else if(strcmp(reply, "RMA NLG\n") == 0){
        printf("%s is not logged in.\n", UID);
    } else {
        printf("%s's Auctions: %s\n", UID, reply);
    }
}

void myBids(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMB %s\n", UID);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if (strcmp(reply, "RMB NOK\n") == 0){
        printf("%s has no bids.\n", UID);
    } else if(strcmp(reply, "RMB NLG\n") == 0){
        printf("%s is not logged in.\n", UID);
    } else {
        printf("%s's Bids: %s\n", UID, reply);
    }
}

void listAuctions(char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LST\n");

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if(strcmp(reply, "RLS NOK\n") == 0){
        printf("No auctions have been started.\n");
    } else {
        printf("%s", reply);
    }
}

void showAsset(int AID, char* ASIP, char* ASport) {
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

void bid(char* UID, char* password, int AID, int value, char* ASIP, char* ASport){
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

void showRecord(int AID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "SRC %d\n", AID);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if(strcmp(reply, "RRC NOK\n") == 0){
        printf("The auction %d does not exist\n", AID);
    } else {
        printf("%s", reply);
    }
}

void logout(char* UID, char* password, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LOU %s %s\n", UID, password);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    printf("%d", strcmp(reply, "RLO OK\n"));

    // Process reply and display results
    if(strcmp(reply, "RLO OK\n") == 0){
        printf("Logout Result: Successful!\n");
    } else if (strcmp(reply, "RLO NOK\n") == 0){
        printf("Logout Result: Unsuccessful, user is not logged in\n");
    } else if(strcmp(reply, "RLO UNR\n") == 0){
        printf("Logout Result: Unrecognized user\n");
    }
}

void unregister(char* UID, char* password, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "UNR %s %s\n", UID, password);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if(strcmp(reply, "RUR OK\n") == 0){
        printf("Unregister Result: Successful!\n");
    } else if (strcmp(reply, "RUR NOK\n") == 0){
        printf("Unregister Result: Unsuccessful, user is not logged in\n");
    } else if(strcmp(reply, "RUR UNR\n") == 0){
        printf("Unregister Result: Unrecognized user\n");
    }
}

void exitApplication() {
    // Implementation
}
