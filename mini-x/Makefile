# Makefile para o projeto mini-X
# Serve para facilitar a compilação dos programas
# Define regras para compilar o servidor e os dois clientes
# Usando o comando 'make' ele compila tudo automaticamente
# E com 'make clean' remove os executáveis gerados

# Definição do compilador a ser usado
CC = gcc

# Flags para compilação
# -Wall ativa todos os avisos importantes
# -g inclui informações para depuração (debug)
# -Iinclude indica onde encontrar os headers (.h)
CFLAGS = -Wall -g -Iinclude

# Flags para linkedição (se precisar adicionar bibliotecas, por exemplo)
# Não foi necessário incluir nenhuma biblioteca extra ou flag de linkedição
LDFLAGS =

# Nomes dos executáveis que serão gerados
SERVER = servidor
CLIENT_SEND = cliente_envio
CLIENT_RECV = cliente_exibicao

# Arquivos fonte correspondentes a cada programa (caminho relativo na pasta src)
SERVER_SRC = src/1_servidor.c src/4_util.c
CLIENT_SEND_SRC = src/2_cliente_envio.c src/4_util.c
CLIENT_RECV_SRC = src/3_cliente_exibicao.c src/4_util.c

# Compila todos os programas (servidor e clientes)
all: $(SERVER) $(CLIENT_SEND) $(CLIENT_RECV)

# O executável servidor depende dos arquivos fonte do servidor
$(SERVER): $(SERVER_SRC)
	# Compila o servidor usando os arquivos fonte
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Executável cliente envio depende dos arquivos fonte dele
$(CLIENT_SEND): $(CLIENT_SEND_SRC)
	# Compila o cliente de envio
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Executável cliente exibição depende dos arquivos fonte dele
$(CLIENT_RECV): $(CLIENT_RECV_SRC)
	# Compila o cliente de exibição
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Comando para apagar executáveis e arquivos objeto
clean:
	rm -f $(SERVER) $(CLIENT_SEND) $(CLIENT_RECV) *.o

# Declara que all e clean são comandos, não arquivos
.PHONY: all clean