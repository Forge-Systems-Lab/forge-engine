#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../shared/server.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "🏛️  [FORGE-CLI] Network Client Active\n");
        fprintf(stderr, "    Usage: %s <STATUS | METRICS | RUN>\n", argv[0]);
        return 1;
    }

    int socket_fd;
    struct sockaddr_in server_address;
    char response_buffer[BUFFER_SIZE];
    char request_buffer[BUFFER_SIZE] = {0};

    if (strcmp(argv[1], "METRICS") == 0 || strcmp(argv[1], "METRICS_JSON") == 0) {
        snprintf(request_buffer, sizeof(request_buffer), 
                 "GET /metrics HTTP/1.1\r\n"
                 "Host: localhost:%d\r\n"
                 "Connection: close\r\n\r\n", PORT);
    } 
    else if (strcmp(argv[1], "RUN") == 0) {
        snprintf(request_buffer, sizeof(request_buffer), 
                 "POST /run HTTP/1.1\r\n"
                 "Host: localhost:%d\r\n"
                 "Connection: close\r\n\r\n", PORT);
    } 
    else {
        snprintf(request_buffer, sizeof(request_buffer), 
                 "GET /status HTTP/1.1\r\n"
                 "Host: localhost:%d\r\n"
                 "Connection: close\r\n\r\n", PORT);
    }

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("[-] Network CLI Error: Socket creation failed");
        return 1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "[-] Network CLI Error: Unable to reach broker web gateway on port %d\n", PORT);
        fprintf(stderr, "[*] Ensure forge-broker daemon is actively running.\n");
        close(socket_fd);
        return 1;
    }

    if (send(socket_fd, request_buffer, strlen(request_buffer), 0) < 0) {
        perror("[-] Network CLI Error: Frame delivery failed");
        close(socket_fd);
        return 1;
    }

    memset(response_buffer, 0, BUFFER_SIZE);
    ssize_t bytes_received = recv(socket_fd, response_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        printf("%s", response_buffer);
    }

    close(socket_fd);
    return 0;
}
