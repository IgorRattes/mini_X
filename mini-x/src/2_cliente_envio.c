// cliente_envio.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")  // No Visual Studio; no MinGW, pode ignorar
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

// Função para fechar socket corretamente conforme plataforma
void fecha_socket(int sockfd) {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "Erro ao iniciar Winsock.\n");
        return 1;
    }
#endif

    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ID> <IP_SERVIDOR> <PORTA>\n", argv[0]);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    int id_cliente = atoi(argv[1]);
    char *ip_servidor = argv[2];
    int porta = atoi(argv[3]);

    if (id_cliente < 1001 || id_cliente > 1999) {
        fprintf(stderr, "Erro: ID do cliente de envio deve estar entre 1001 e 1999.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Criação do socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) erro("socket");

    // Configuração do endereço do servidor
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Correção: zerar antes
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(porta);

    if (inet_pton(AF_INET, ip_servidor, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Erro: IP do servidor inválido.\n");
        fecha_socket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Conexão com o servidor
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
#ifdef _WIN32
        fprintf(stderr, "Erro no connect: %d\n", WSAGetLastError());
#else
        perror("connect");
#endif
        fecha_socket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    // Envio da mensagem OI
    Mensagem oi;
    oi.tipo = MSG_OI;
    oi.id_origem = id_cliente;
    oi.id_destino = 0;
    memset(oi.texto, 0, TAM_TEXTO); // garantir zerar a string

    if (send(sockfd, (char*)&oi, sizeof(Mensagem), 0) < 0)
        erro("send OI");

    // Espera resposta do servidor
    Mensagem resposta;
    int recebido = recv(sockfd, (char*)&resposta, sizeof(Mensagem), 0);
    if (recebido <= 0) {
        fprintf(stderr, "Erro ao receber resposta do servidor. Encerrando.\n");
        fecha_socket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    if (resposta.tipo != MSG_OI || resposta.id_origem != id_cliente) {
        fprintf(stderr, "Servidor rejeitou o ID ou respondeu incorretamente.\n");
        fecha_socket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    printf("Conectado como cliente de envio (ID %d)\n", id_cliente);

    // Loop principal para envio de mensagens
    while (1) {
        Mensagem msg;
        msg.tipo = MSG_MSG;
        msg.id_origem = id_cliente;

        printf("\nDigite o ID de destino (0 para todos, -1 para sair): ");
        fflush(stdout);
        if (scanf("%d", &msg.id_destino) != 1) {
            printf("Entrada inválida.\n");
            while (getchar() != '\n'); // limpa buffer stdin
            continue;
        }
        getchar(); // Consome o '\n' restante

        if (msg.id_destino == -1) {
            msg.tipo = MSG_TCHAU;
            if (send(sockfd, (char*)&msg, sizeof(Mensagem), 0) < 0)
                perror("Erro ao enviar TCHAU");
            break;
        }

        printf("Digite a mensagem: ");
        fflush(stdout);
        if (!fgets(msg.texto, TAM_TEXTO, stdin)) {
            printf("Erro ao ler mensagem.\n");
            continue;
        }
        msg.texto[strcspn(msg.texto, "\n")] = '\0'; // Remove newline

        if (strlen(msg.texto) == 0) {
            printf("Mensagem vazia não será enviada.\n");
            continue;
        }

        if (send(sockfd, (char*)&msg, sizeof(Mensagem), 0) < 0) {
            perror("Erro ao enviar mensagem");
            break;
        }
    }

    fecha_socket(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
    printf("Desconectado.\n");
    return 0;
}
