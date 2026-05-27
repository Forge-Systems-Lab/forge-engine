CC = gcc
CFLAGS = -Wall -Wextra -O3 -mavx2 -march=native -pthread -Iinclude

# Source file arrays mapped to their respective binaries
BROKER_SRC = src/main.c src/queue.c src/parser.c src/telemetry.c src/server.c
CLI_SRC    = src/cli.c

BROKER_OBJ = $(BROKER_SRC:.c=.o)
CLI_OBJ    = $(CLI_SRC:.c=.o)

# Target outputs
BROKER_TARGET = forge-broker
CLI_TARGET    = forge-cli

all: $(BROKER_TARGET) $(CLI_TARGET)

$(BROKER_TARGET): $(BROKER_OBJ)
	$(CC) $(CFLAGS) -o $(BROKER_TARGET) $(BROKER_OBJ)

$(CLI_TARGET): $(CLI_OBJ)
	$(CC) $(CFLAGS) -o $(CLI_TARGET) $(CLI_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BROKER_TARGET) $(CLI_TARGET)
