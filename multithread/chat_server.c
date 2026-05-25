#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 100

typedef struct {
    int fd;
    char name[50];
    int registered;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int nClients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    int client_idx = -1;

    char *welcome = "Hay nhap ten theo cu phap: client_id: client_name\n";
    send(client_fd, welcome, strlen(welcome), 0);

    char buf[256];
    while (1) {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = 0;

        pthread_mutex_lock(&clients_mutex);
        // Tìm index của client hiện tại
        for(int i=0; i<nClients; i++) if(clients[i].fd == client_fd) { client_idx = i; break; }

        if (!clients[client_idx].registered) {
            char name[50];
            if (sscanf(buf, "client_id: %s", name) == 1) {
                strcpy(clients[client_idx].name, name);
                clients[client_idx].registered = 1;
                send(client_fd, "Dang nhap thanh cong!\n", 22, 0);
            } else {
                send(client_fd, "Sai cu phap. Nhap lai!\n", 23, 0);
            }
        } else {
            // Logic Broadcast kèm thời gian
            time_t rawtime;
            struct tm *ti;
            char time_str[25], send_buf[512];
            time(&rawtime);
            ti = localtime(&rawtime);
            strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", ti);
            sprintf(send_buf, "%s %s: %s", time_str, clients[client_idx].name, buf);
            
            for (int j = 0; j < nClients; j++) {
                if (clients[j].registered && clients[j].fd != client_fd) {
                    send(clients[j].fd, send_buf, strlen(send_buf), 0);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    // Xử lý khi thoát
    close(client_fd);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < nClients; i++) {
        if (clients[i].fd == client_fd) {
            clients[i] = clients[nClients - 1];
            nClients--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8080), htonl(INADDR_ANY)};
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        pthread_mutex_lock(&clients_mutex);
        if (nClients < MAX_CLIENTS) {
            clients[nClients].fd = client_fd;
            clients[nClients].registered = 0;
            nClients++;
            int *new_sock = malloc(sizeof(int));
            *new_sock = client_fd;
            pthread_t tid;
            pthread_create(&tid, NULL, client_handler, new_sock);
            pthread_detach(tid);
        } else {
            close(client_fd);
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return 0;
}