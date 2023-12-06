#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "communication.h"

void sendUDPMessage(const char* message, char* reply, char* ASPort, char* ASIP);
void sendTCPMessage(const char* message, char* reply, char* ASPort, char* ASIP);

void login(char* UID, char* password, char* ASIP, char* ASPort);
void openAuction(char* UID, char* password, char* name, char* asset_fname, char* start_value, char* timeactive,char* ASIP, char* ASPort);
void closeAuction(char* UID, char* password, char* AID, char* ASIP, char* ASPort);
void myAuctions(char* UID, char* ASIP, char* ASPort);
void myBids(char* UID, char* ASIP, char* ASPort);
void listAuctions(char* ASIP, char* ASPort);
void showAsset(int AID, char* ASIP, char* ASPort);
void bid(char* UID, char* password, char* AID, char* value, char* ASIP, char* ASPort);
void showRecord(char* AID, char* ASIP, char* ASPort);
void logout(char* UID, char* password, char* ASIP, char* ASPort);
void unregister(char* UID, char* password, char* ASIP, char* ASPort);
void exitApplication();
char *readFile(const char *filename);

int main(int argc, char *argv[]) {
    char ASIP[50] = "tejo.tecnico.ulisboa.pt"; //TODO DEBUG
    char ASport[6] = "58011";

    char input[50];

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

    // Application loop
    while (1) {
        char token[50];
        char UID[7];
        char password[9];

        printf("Digite um comando: ");
        scanf("%s", token);

        // Compare command and call the corresponding function
        if (strcmp(token, "login") == 0) {
            scanf("%s %s", UID, password); //TODO não sei se podemos associar já aqui, maybe deviamos esperar pela reposta
            login(UID, password, ASIP, ASport);

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
            int AID = atoi(strtok(NULL, " "));
            showAsset(AID, ASIP, ASport);

        } else if (strcmp(token, "bid") == 0) {
            char AID[4];
            char value[MAX_BUFFER_SIZE];
            scanf("%s %s", AID, value);
            bid(UID, password, AID, value, ASIP , ASport);

        } else if (strcmp(token, "show_record") == 0 || strcmp(token, "sr") == 0) {
            char AID[4];
            scanf("%s", AID);
            showRecord(AID, ASIP, ASport);

        } else if (strcmp(token, "logout") == 0) {
            logout(UID, password, ASIP, ASport);

        } else if (strcmp(token, "unregister") == 0) {
            unregister(UID, password, ASIP, ASport);

        } else if (strcmp(token, "exit") == 0) {
            // TODO : verificar se nao ta loged in
            break; // Exit the loop and end the program

        } else {
            printf("Invalid command. Try again.\n");
        }
    }

    return 0;    
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
    /* Cria um socket UDP (SOCK_DGRAM) para IPv4 (AF_INET).
    É devolvido um descritor de ficheiro (fd) para onde se deve comunicar. */
    fd = createUDPSocket();

    /* Preenche a estrutura com 0s e depois atribui a informação já conhecida da ligação */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    /* Busca informação do host ASPort, na porta especificada,
    guardando a informação nas `hints` e na `res`. Caso o host seja um nome
    e não um endereço ASIP (como é o caso), efetua um DNS Lookup. */
    errcode = getaddrinfo(ASPort, ASIP, &hints, &res); //TODO ASPort e ASIP nao estao trocados?
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

    // Find the position of the newline character in the received data
    char* newline_pos = strchr(reply, '\n');
    if (newline_pos != NULL) {
        newline_pos++;
        *newline_pos = '\0';
    }

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);
}

void sendTCPMessage(const char* message, char* reply, char* ASPort, char* ASIP) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    /* Cria um socket TCP (SOCK_STREAM) para IPv4 (AF_INET).
    É devolvido um descritor de ficheiro (fd) para onde se deve comunicar. */
    fd = createTCPSocket();
    if (fd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    errcode = getaddrinfo(ASPort, ASIP, &hints, &res);
    if (errcode != 0) {
        exit(1);
    }

    /* Em TCP é necessário estabelecer uma ligação com o servidor primeiro (Handshake).
    Então primeiro cria a conexão para o endereço obtido através de `getaddrinfo()`. */
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        exit(1);
    }

    /* Escreve a mensagem "Hello!\n" para o servidor, especificando o seu tamanho */
    n=write(fd, message, strlen(message));
    if (n == -1) {
        exit(1);
    }

    /* Lê 128 Bytes do servidor e guarda-os no buffer. */
    n=read(fd, reply, 128);
    if (n == -1) {
        exit(1);
    }

    // Find the position of the newline character in the received data
    char* newline_pos = strchr(reply, '\n');
    if (newline_pos != NULL) {
        newline_pos++;
        *newline_pos = '\0';
    }

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);
}

// User Actions Functions
void login(char* UID, char* password, char* ASIP, char* ASPort) {
    
    char loginMessage[MAX_BUFFER_SIZE];
    snprintf(loginMessage, sizeof(loginMessage), "LIN %s %s\n", UID, password);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(loginMessage, reply, ASIP, ASPort);

    // Process reply and display results
    if(strcmp(reply, "RLI OK\n") == 0){
        printf("Login Result: Successful!\n");
    } else if (strcmp(reply, "RLI NOK\n") == 0){
        printf("Login Result: Unsuccessful :(\n");
    } else if(strcmp(reply, "RLI REG\n") == 0){
        printf("Login Result: User Registered!\n");
    }
}

void openAuction(char* UID, char* password, char* name, char* asset_fname, char* start_value, char* timeactive, char* ASIP, char* ASPort) {
    // Prepare open auction message
    char openAuctionMessage[MAX_BUFFER_SIZE];

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

    char *fileData = readFile(asset_fname);

    snprintf(openAuctionMessage, sizeof(openAuctionMessage), "OPA %s %s %s %s %s %s %ld %s\n", UID, password, name, start_value, timeactive, asset_fname, fsize, fileData);

    printf("%s", openAuctionMessage);

    // Read the file data
    fread(openAuctionMessage + strlen(openAuctionMessage), 1, fsize, assetFile);
    fclose(assetFile);

    // Send open auction message
    char reply[MAX_BUFFER_SIZE];
    sendTCPMessage(openAuctionMessage, reply, ASIP, ASPort);

    // Process reply and display results
    if (strncmp(reply, "ROA OK", 6) == 0) {
        // Auction opened successfully
        int AID;
        sscanf(reply + 7, "%d", &AID);
        printf("Auction opened successfully! Auction ID: %d\n", AID);
    } else if (strncmp(reply, "ROA NOK", 7) == 0) {
        printf("Error opening auction: %s\n", reply + 8);
    } else if (strncmp(reply, "ROA NLG", 7) == 0) {
        printf("Error: User not logged in.\n");
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
    long fileSize = ftell(file);
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
    sendTCPMessage(closeAuctionMessage, reply, ASIP, ASPort);

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


void myAuctions(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMA %s\n", UID);

    printf("DEBUG message: %s\n", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if (strcmp(reply, "RMA NOK\n") == 0){
        printf("%s has no auctions\n", UID);
    } else if(strcmp(reply, "RMA NLG\n") == 0){
        printf("%s is not logged in.\n", UID);
    } else {
        printf("%s's Auctions: %s\n", UID, reply); //TODO: maybe será preciso imprimir melhor?
    }
}

void myBids(char* UID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LMB %s\n", UID);

    printf("DEBUG message: %s\n", message);

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

void showAsset(int AID, char* ASIP, char* ASPort) {
    // Prepare show asset message
    char showAssetMessage[MAX_BUFFER_SIZE];
    snprintf(showAssetMessage, sizeof(showAssetMessage), "SAS %d", AID);

    char reply[MAX_BUFFER_SIZE];
    sendTCPMessage(showAssetMessage, reply, ASIP, ASPort);

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
    // Prepare bid message
    char bidMessage[MAX_BUFFER_SIZE];
    snprintf(bidMessage, sizeof(bidMessage), "BID %s %s %s %s\n", UID, password, AID, value);

    // Send bid message
    char reply[MAX_BUFFER_SIZE];
    sendTCPMessage(bidMessage, reply, ASIP, ASPort);

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
    }
}


void showRecord(char* AID, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "SRC %s\n", AID);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

    // Process reply and display results
    if(strcmp(reply, "RRC NOK\n") == 0){
        printf("The auction %s does not exist\n", AID);
    } else {
        printf("Show record %s result: %s", AID, reply);
    }
}

void logout(char* UID, char* password, char* ASIP, char* ASPort) {
    char message[MAX_BUFFER_SIZE];
    snprintf(message, sizeof(message), "LOU %s %s\n", UID, password);

    printf("%s", message);

    //envia mensagem
    char reply[MAX_BUFFER_SIZE];
    sendUDPMessage(message, reply, ASIP, ASPort);

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

