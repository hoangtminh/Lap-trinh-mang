#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8082);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("Kết nối thất bại");
        return 1;
    }

    printf("Đã kết nối Telnet Server. Nhập 'user pass' để đăng nhập.\n");

    fd_set fdread;
    char buf[2048];

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(STDIN_FILENO, &fdread);
        FD_SET(client, &fdread);

        select(client + 1, &fdread, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &fdread)) {
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }

        if (FD_ISSET(client, &fdread)) {
            int ret = recv(client, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                printf("Server đã đóng kết nối.\n");
                break;
            }
            buf[ret] = 0;
        }
    }

    close(client);
    return 0;
}