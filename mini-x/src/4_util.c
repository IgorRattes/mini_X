#include "util.h"

#ifdef _WIN32
#include <winsock2.h>
#include <stdio.h>
#else
#include <stdio.h>
#include <errno.h>
#endif

// Imprime erro e finaliza o programa
void erro(const char *msg) {
#ifdef _WIN32
    fprintf(stderr, "%s: %d\n", msg, WSAGetLastError());
#else
    perror(msg);
#endif
    exit(EXIT_FAILURE);
}

// Verifica se o ID está no intervalo válido para cliente de envio
int id_envio_valido(int id) {
    return id >= ID_CLIENTE_ENVIO_MIN && id <= ID_CLIENTE_ENVIO_MAX;
}

// Verifica se o ID está no intervalo válido para cliente de exibição
int id_exibicao_valido(int id) {
    return id >= ID_CLIENTE_EXIBICAO_MIN && id <= ID_CLIENTE_EXIBICAO_MAX;
}
