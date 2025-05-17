// 1_servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

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
    int socket_fd;      // socket do cliente
    int ativo;          // flag para saber se cliente está ativo (1) ou não (0)
    int id;             // identificador único do cliente (usado na comunicação)
} Cliente;

Cliente clientes[MAX_CLIENTS];

// Inicializa array de clientes como inativos
void inicializa_clientes() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientes[i].ativo = 0;
        clientes[i].socket_fd = -1;
        clientes[i].id = -1;
    }
}

// Adiciona cliente novo, retorna índice ou -1 se cheio
int adiciona_cliente(int socket_fd) {
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
void remove_cliente(int socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientes[i].ativo && clientes[i].socket_fd == socket_fd) {
            close(clientes[i].socket_fd);
            clientes[i].ativo = 0;
            clientes[i].socket_fd = -1;
            clientes[i].id = -1;
            printf("Cliente no socket %d removido\n", socket_fd);
            break;
        }
    }
}

// Envia mensagem para um cliente
void envia_mensagem(int socket_fd, Mensagem *msg) {
    send(socket_fd, msg, sizeof(Mensagem), 0);
}

// Procura índice do cliente pelo socket
int cliente_por_socket(int socket_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientes[i].ativo && clientes[i].socket_fd == socket_fd) {
            return i;
        }
    }
    return -1;
}

// Processa a mensagem recebida do cliente
void processa_mensagem(int socket_fd, Mensagem *msg) {
    int idx = cliente_por_socket(socket_fd);
    if (idx == -1) {
        printf("Mensagem de socket não identificado %d\n", socket_fd);
        return;
    }

    switch (msg->tipo) {
        case MSG_OI:
            // Cliente tentando se identificar
            printf("Cliente no socket %d enviou OI com id %d\n", socket_fd, msg->id_origem);
            // Se já tem id definido, não aceitar novo OI
            if (clientes[idx].id == -1) {
                clientes[idx].id = msg->id_origem;
                // envia confirmação igual (OI)
                envia_mensagem(socket_fd, msg);
                printf("Cliente %d identificado e aceito\n", msg->id_origem);
            } else {
                // cliente já identificado, fechar conexão
                printf("Cliente %d já identificado, fechando conexão\n", clientes[idx].id);
                remove_cliente(socket_fd);
            }
            break;

        case MSG_TCHAU:
            // Cliente quer desconectar
            printf("Cliente %d pediu TCHAU\n", clientes[idx].id);
            remove_cliente(socket_fd);
            break;

        case MSG_MSG:
            // Mensagem enviada do cliente para outro (ou broadcast)
            printf("Cliente %d enviou mensagem para %d: %s\n",
                   clientes[idx].id, msg->id_destino, msg->texto);
            // Validar se id de origem bate com o cliente identificado
            if (msg->id_origem != clientes[idx].id) {
                printf("ID origem inválido na mensagem, desconectando cliente\n");
                remove_cliente(socket_fd);
                break;
            }
            // Enviar mensagem para cliente(s) destino
            if (msg->id_destino == 0) {
                // broadcast para todos clientes exibidores
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clientes[i].ativo && clientes[i].id != clientes[idx].id) {
                        envia_mensagem(clientes[i].socket_fd, msg);
                    }
                }
            } else {
                // enviar para cliente exibição especifico
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
    int listener_fd, new_fd;              // Socket do servidor e socket novo para o cliente
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    fd_set master_fds, read_fds;          // Conjuntos de sockets para usar com select()
    int fdmax;                            // Maior descritor de arquivo usado no select()
    int i;

    // Inicializa array de clientes
    inicializa_clientes();

    // --- Inicialização do servidor ---

    listener_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener_fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(listener_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listener_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    FD_SET(listener_fd, &master_fds);
    fdmax = listener_fd;

    printf("Servidor escutando na porta %d\n", PORT);

    while(1) {
        read_fds = master_fds;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener_fd) {
                    // Nova conexão
                    addrlen = sizeof(client_addr);
                    new_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addrlen);
                    if (new_fd == -1) {
                        perror("accept");
                    } else {
                        // Adiciona novo cliente
                        if (adiciona_cliente(new_fd) == -1) {
                            printf("Servidor cheio, rejeitando conexão\n");
                            close(new_fd);
                        } else {
                            FD_SET(new_fd, &master_fds);
                            if (new_fd > fdmax) fdmax = new_fd;
                            printf("Nova conexão de %s na socket %d\n",
                                   inet_ntoa(client_addr.sin_addr), new_fd);
                        }
                    }
                } else {
                    // Dados recebidos de cliente
                    Mensagem msg;
                    int nbytes = recv(i, &msg, sizeof(Mensagem), 0);
                    if (nbytes <= 0) {
                        if (nbytes == 0) {
                            printf("Socket %d desconectou\n", i);
                        } else {
                            perror("recv");
                        }
                        remove_cliente(i);
                        FD_CLR(i, &master_fds);
                    } else {
                        processa_mensagem(i, &msg);
                    }
                }
            }
        }
    }

    return 0;
}
