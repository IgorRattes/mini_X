// 1_servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>  // Para inet_pton, IPv6, etc.

#pragma comment(lib, "ws2_32.lib") // Apenas para Visual Studio, pode ignorar no MinGW

#define PORT 12345           // Porta que o servidor vai escutar
#define MAX_CLIENTS 20       // Limite máximo de clientes conectados

// Tipos de mensagem segundo o protocolo
#define MSG_OI 0
#define MSG_TCHAU 1
#define MSG_MSG 2

// Estrutura da mensagem para troca entre cliente e servidor
typedef struct {
    int tipo;           // tipo da mensagem: OI, TCHAU, MSG
    int id_origem;      // id do cliente que envia (preenchido no OI)
    int id_destino;     // id do cliente destino (0 para broadcast)
    char texto[256];    // conteúdo da mensagem (texto ou vazio)
} Mensagem;

// Estrutura para armazenar clientes conectados
typedef struct {
    SOCKET socket_fd;   // socket do cliente (SOCKET no Windows)
    int ativo;          // flag para saber se cliente está ativo (1) ou não (0)
    int id;             // identificador único do cliente (usado na comunicação)
} Cliente;

Cliente clientes[MAX_CLIENTS];

// Inicializa array de clientes como inativos
void inicializa_clientes() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientes[i].ativo = 0;
        clientes[i].socket_fd = INVALID_SOCKET;
        clientes[i].id = -1;
    }
}

// Adiciona cliente novo, retorna índice ou -1 se cheio
int adiciona_cliente(SOCKET socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clientes[i].ativo) {
            clientes[i].ativo = 1;
            clientes[i].socket_fd = socket_fd;
            clientes[i].id = -1; // ainda sem id definido, espera OI
            return i;
        }
    }
    return -1; // cheio
}

// Remove cliente do array e fecha socket
void remove_cliente(SOCKET socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientes[i].ativo && clientes[i].socket_fd == socket_fd) {
            closesocket(clientes[i].socket_fd);
            clientes[i].ativo = 0;
            clientes[i].socket_fd = INVALID_SOCKET;
            clientes[i].id = -1;
            printf("Cliente no socket %llu removido\n", (unsigned long long)socket_fd);
            break;
        }
    }
}

// Envia mensagem para um cliente
void envia_mensagem(SOCKET socket_fd, Mensagem *msg) {
    // CAST EXPLÍCITO PARA EVITAR WARNING:
    send(socket_fd, (const char*)msg, sizeof(Mensagem), 0);
}

// Procura índice do cliente pelo socket
int cliente_por_socket(SOCKET socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientes[i].ativo && clientes[i].socket_fd == socket_fd) {
            return i;
        }
    }
    return -1;
}

// Processa a mensagem recebida do cliente
void processa_mensagem(SOCKET socket_fd, Mensagem *msg) {
    int idx = cliente_por_socket(socket_fd);
    if (idx == -1) {
        printf("Mensagem de socket não identificado %llu\n", (unsigned long long)socket_fd);
        return;
    }

    switch (msg->tipo) {
        case MSG_OI:
            // Cliente tentando se identificar
            printf("Cliente no socket %llu enviou OI com id %d\n", (unsigned long long)socket_fd, msg->id_origem);
            if (clientes[idx].id == -1) {
                clientes[idx].id = msg->id_origem;
                envia_mensagem(socket_fd, msg);
                printf("Cliente %d identificado e aceito\n", msg->id_origem);
            } else {
                printf("Cliente %d já identificado, fechando conexão\n", clientes[idx].id);
                remove_cliente(socket_fd);
            }
            break;

        case MSG_TCHAU:
            printf("Cliente %d pediu TCHAU\n", clientes[idx].id);
            remove_cliente(socket_fd);
            break;

        case MSG_MSG:
            printf("Cliente %d enviou mensagem para %d: %s\n",
                   clientes[idx].id, msg->id_destino, msg->texto);
            if (msg->id_origem != clientes[idx].id) {
                printf("ID origem inválido na mensagem, desconectando cliente\n");
                remove_cliente(socket_fd);
                break;
            }
            if (msg->id_destino == 0) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clientes[i].ativo && clientes[i].id != clientes[idx].id) {
                        envia_mensagem(clientes[i].socket_fd, msg);
                    }
                }
            } else {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clientes[i].ativo && clientes[i].id == msg->id_destino) {
                        envia_mensagem(clientes[i].socket_fd, msg);
                        break;
                    }
                }
            }
            break;

        default:
            printf("Mensagem com tipo desconhecido recebida\n");
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "Erro ao iniciar Winsock.\n");
        return 1;
    }

    SOCKET listener_fd, new_fd;
    struct sockaddr_in server_addr, client_addr;
    int addrlen;
    fd_set master_fds, read_fds;
    int fdmax;
    int i;

    inicializa_clientes();

    listener_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener_fd == INVALID_SOCKET) {
        fprintf(stderr, "Erro ao criar socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // No Windows, SO_REUSEADDR pode ser um int
    int yes = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes)) == SOCKET_ERROR) {
        fprintf(stderr, "Erro no setsockopt: %d\n", WSAGetLastError());
        closesocket(listener_fd);
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Erro no bind: %d\n", WSAGetLastError());
        closesocket(listener_fd);
        WSACleanup();
        return 1;
    }

    if (listen(listener_fd, 10) == SOCKET_ERROR) {
        fprintf(stderr, "Erro no listen: %d\n", WSAGetLastError());
        closesocket(listener_fd);
        WSACleanup();
        return 1;
    }

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    FD_SET(listener_fd, &master_fds);
    fdmax = (int)listener_fd;

    printf("Servidor escutando na porta %d\n", PORT);

    while (1) {
        read_fds = master_fds;

        int select_result = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (select_result == SOCKET_ERROR) {
            fprintf(stderr, "Erro no select: %d\n", WSAGetLastError());
            break;
        }

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if ((SOCKET)i == listener_fd) {
                    addrlen = sizeof(client_addr);
                    new_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addrlen);
                    if (new_fd == INVALID_SOCKET) {
                        fprintf(stderr, "Erro no accept: %d\n", WSAGetLastError());
                    } else {
                        if (adiciona_cliente(new_fd) == -1) {
                            printf("Servidor cheio, rejeitando conexão\n");
                            closesocket(new_fd);
                        } else {
                            FD_SET(new_fd, &master_fds);
                            if ((int)new_fd > fdmax) fdmax = (int)new_fd;
                            printf("Nova conexão de %s na socket %llu\n",
                                   inet_ntoa(client_addr.sin_addr), (unsigned long long)new_fd);
                        }
                    }
                } else {
                    Mensagem msg;
                    // CAST EXPLÍCITO PARA EVITAR WARNING:
                    int nbytes = recv((SOCKET)i, (char*)&msg, sizeof(Mensagem), 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("Socket %d desconectou\n", i);
                        } else {
                            fprintf(stderr, "Erro no recv: %d\n", WSAGetLastError());
                        }
                        remove_cliente((SOCKET)i);
                        FD_CLR(i, &master_fds);
                    } else {
                        processa_mensagem((SOCKET)i, &msg);
                    }
                }
            }
        }
    }

    closesocket(listener_fd);
    WSACleanup();
    return 0;
}
