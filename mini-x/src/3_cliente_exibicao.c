// cliente_exibicao.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "util.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

// Endereço e porta do servidor
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

#ifdef _WIN32
SOCKET sockfd = INVALID_SOCKET;  // socket global
#else
int sockfd = -1;
#endif

// Função chamada ao pressionar Ctrl+C
void envia_tchau_e_sai(int sig) {
    if (
#ifdef _WIN32
        sockfd != INVALID_SOCKET
#else
        sockfd != -1
#endif
    ) {
        Mensagem msg = {0};
        msg.tipo = MSG_TCHAU;
        msg.id_origem = 0;
        msg.id_destino = 0;
        send(sockfd, (char*)&msg, sizeof(Mensagem), 0);
        printf("\n[Cliente] Enviando TCHAU e encerrando conexão.\n");
#ifdef _WIN32
        closesocket(sockfd);
        // Não chamar WSACleanup aqui
        sockfd = INVALID_SOCKET;
#else
        close(sockfd);
        sockfd = -1;
#endif
    }
    exit(0);
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "Erro ao iniciar Winsock.\n");
        return 1;
    }
#endif

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ID_CLIENTE>\n", argv[0]);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    int meu_id = atoi(argv[1]);

    // Verifica se o ID está no intervalo reservado para clientes exibidores
    if (meu_id < 2001 || meu_id > 2999) {
        fprintf(stderr, "Erro: ID do cliente exibidor deve estar entre 2001 e 2999.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Instala handler de sinal para Ctrl+C (SIGINT)
    signal(SIGINT, envia_tchau_e_sai);

    // Cria o socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sockfd == INVALID_SOCKET) erro("socket");
#else
    if (sockfd < 0) erro("socket");
#endif

    // Define endereço do servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Zera toda a struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Erro: IP do servidor inválido.\n");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        exit(EXIT_FAILURE);
    }

    // Conecta ao servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
#ifdef _WIN32
        erro("connect");
#else
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
#endif
    }

    // Envia mensagem OI
    Mensagem msg;
    msg.tipo = MSG_OI;
    msg.id_origem = meu_id;
    msg.id_destino = 0;
    memset(msg.texto, 0, sizeof(msg.texto));

    if (send(sockfd, (char*)&msg, sizeof(Mensagem), 0) < 0) {
        perror("Erro ao enviar OI");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        exit(EXIT_FAILURE);
    }

    // Espera resposta do servidor
    if (recv(sockfd, (char*)&msg, sizeof(Mensagem), 0) <= 0) {
        fprintf(stderr, "Erro ao receber resposta do servidor.\n");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        exit(EXIT_FAILURE);
    }

    // Verifica se o servidor aceitou a conexão
    if (msg.tipo != MSG_OI || msg.id_origem != meu_id) {
        fprintf(stderr, "Servidor rejeitou o identificador ou respondeu incorretamente.\n");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        exit(EXIT_FAILURE);
    }

    printf("Conectado como exibidor (ID %d). Aguardando mensagens...\n", meu_id);

    // Loop principal para receber mensagens
    while (1) {
        int n = recv(sockfd, (char*)&msg, sizeof(Mensagem), 0);
        if (n <= 0) {
            printf("Conexão encerrada pelo servidor.\n");
            break;
        }

        if (msg.tipo == MSG_MSG) {
            printf("[Mensagem de %d]: %s\n", msg.id_origem, msg.texto);
        }
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return 0;
}
