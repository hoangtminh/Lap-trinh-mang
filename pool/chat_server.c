#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
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
    struct pollfd fds[MAX_CLIENTS + 1];
    int nClients = 0;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    printf("Server dang chay tai port 8080...\n");

    while (1) {
        for (int i = 0; i < nClients; i++) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = POLLIN;
        }
        int poll_count = poll(fds, nClients + 1, -1);

        if (poll_count < 0) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = client_fd;
                clients[nClients].registered = 0;
                nClients++;
                
                char *msg = "Hay nhap ten theo cu phap: client_id: client_name\n";
                send(client_fd, msg, strlen(msg), 0);
                printf("Client moi ket noi: FD %d\n", client_fd);
            } else {
                close(client_fd);
            }
        }
        for (int i = 0; i < nClients; i++) {
            if (fds[i + 1].revents & POLLIN) {
                char buf[256];
                int ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

                if (ret <= 0) {
                    printf("Client %s (FD %d) da thoat.\n", clients[i].name, clients[i].fd);
                    close(clients[i].fd);
                    clients[i] = clients[nClients - 1];
                    nClients--;
                    i--; 
                    continue;
                }

                buf[ret] = 0;
                if (!clients[i].registered) {
                    char name[50];
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
                        if (clients[j].registered && j != i) {
                            send(clients[j].fd, send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);
    return 0;
}