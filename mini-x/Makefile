# Makefile para o projeto mini-X
# Responsável por compilar o servidor e os dois clientes (envio e exibição)
# Comando 'make' compila todos os binários
# Comando 'make clean' remove os executáveis gerados

# Compilador a ser utilizado
CC = gcc

# Flags de compilação:
# -Wall: ativa todos os avisos importantes
# -g: inclui informações de debug
# -Iinclude: adiciona o diretório 'include' à lista de paths de headers
CFLAGS = -Wall -g -Iinclude

# Flags de linkedição (vazio por enquanto, pode adicionar libs aqui se necessário)
LDFLAGS = -lws2_32

# Nomes dos executáveis a serem gerados
SERVER = servidor
CLIENT_SEND = cliente_envio
CLIENT_RECV = cliente_exibicao

# Arquivos-fonte utilizados por cada executável
SERVER_SRC = src/1_servidor.c src/4_util.c
CLIENT_SEND_SRC = src/2_cliente_envio.c src/4_util.c
CLIENT_RECV_SRC = src/3_cliente_exibicao.c src/4_util.c

# Regra principal: compila todos os programas
all: $(SERVER) $(CLIENT_SEND) $(CLIENT_RECV)

# Regras de compilação individuais para cada binário

# Compila o servidor
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compila o cliente de envio
$(CLIENT_SEND): $(CLIENT_SEND_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compila o cliente de exibição
$(CLIENT_RECV): $(CLIENT_RECV_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Remove arquivos gerados na compilação
clean:
	rm -f $(SERVER) $(CLIENT_SEND) $(CLIENT_RECV) *.o

# Indica que 'all' e 'clean' não são arquivos
.PHONY: all clean