#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9000

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ket noi that bai");
        return 1;
    }

    char buf[1024];
    fd_set fdread;

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(STDIN_FILENO, &fdread);
        FD_SET(client_fd, &fdread);

        int max_fd = (client_fd > STDIN_FILENO) ? client_fd : STDIN_FILENO;

        if (select(max_fd + 1, &fdread, NULL, NULL, NULL) < 0) {
            perror("select() error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            if (fgets(buf, sizeof(buf), stdin)) {
                send(client_fd, buf, strlen(buf), 0);
            }
        }

        if (FD_ISSET(client_fd, &fdread)) {
            int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                printf("Server da ngat ket noi.\n");
                break;
            }
            buf[ret] = '\0';
            printf("%s", buf);
        }
    }

    close(client_fd);
    return 0;
}