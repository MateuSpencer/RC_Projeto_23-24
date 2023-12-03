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

    /* Desaloca a memória da estrutura `res` e fecha o socket */
    freeaddrinfo(res);
    close(fd);
}