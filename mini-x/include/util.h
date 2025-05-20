#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#endif

// Definições dos tipos de mensagem
#define MSG_OI    0
#define MSG_TCHAU 1
#define MSG_MSG   2

// Intervalos de ID
#define ID_CLIENTE_ENVIO_MIN    1001
#define ID_CLIENTE_ENVIO_MAX    1999
#define ID_CLIENTE_EXIBICAO_MIN 2001
#define ID_CLIENTE_EXIBICAO_MAX 2999

// Tamanho máximo do texto da mensagem
#define TAM_TEXTO 141

// Estrutura da mensagem trocada entre clientes e servidor
typedef struct {
    int tipo;
    int id_origem;
    int id_destino;
    char texto[TAM_TEXTO];
} Mensagem;

// Protótipo da função de erro
void erro(const char *msg);

// Protótipos das funções auxiliares para validação de ID
int id_envio_valido(int id);
int id_exibicao_valido(int id);

#endif // UTIL_H
