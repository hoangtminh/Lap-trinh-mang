#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 100

typedef struct {
    int fd;
    char name[50];
    int registered;
} ClientInfo;

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    ClientInfo clients[MAX_CLIENTS];
    int nClients = 0;
    fd_set fdread;

    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int max_fd = listener;

        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i].fd, &fdread);
            if (clients[i].fd > max_fd) max_fd = clients[i].fd;
        }

        select(max_fd + 1, &fdread, NULL, NULL, NULL);

        if (FD_ISSET(listener, &fdread)) {
            int client_fd = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = client_fd;
                clients[nClients].registered = 0;
                nClients++;
                char *msg = "Hay nhap ten theo cu phap: client_id: client_name\n";
                send(client_fd, msg, strlen(msg), 0);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                char buf[256];
                int ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    close(clients[i].fd);
                    clients[i] = clients[nClients - 1];
                    nClients--; i--; continue;
                }
                buf[ret] = 0;

                if (!clients[i].registered) {
                    char id[50], name[50];
                    if (sscanf(buf, "client_id: %s", name) == 1) {
                        strcpy(clients[i].name, name);
                        clients[i].registered = 1;
                        send(clients[i].fd, "Dang nhap thanh cong!\n", 22, 0);
                    } else {
                        send(clients[i].fd, "Sai cu phap. Nhap lai!\n", 23, 0);
                    }
                } else {
                    time_t rawtime;
                    struct tm *timeinfo;
                    char time_str[25];
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", timeinfo);

                    char send_buf[512];
                    sprintf(send_buf, "%s %s: %s", time_str, clients[i].name, buf);
                    
                    for (int j = 0; j < nClients; j++) {
                        if (clients[j].registered && j != i) 
                            send(clients[j].fd, send_buf, strlen(send_buf), 0);
                    }
                }
            }
        }
    }
    return 0;
}