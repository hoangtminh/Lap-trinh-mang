#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 64

int clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *client_thread(void *param) {
    int client = *(int *)param;
    char buf[256];
    
    char *welcome = "Chao mung vao phong chat chung!\n";
    send(client, welcome, strlen(welcome), 0);

    while (1) {
        int len = recv(client, buf, sizeof(buf) - 1, 0);
        if (len <= 0) break;
        
        buf[len] = '\0';
        
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_clients; i++) {
            if (clients[i] != client) {
                send(clients[i], buf, len, 0);
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    
    close(client);
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            clients[i] = clients[num_clients - 1];
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    printf("Client %d da thoat. Con %d client.\n", client, num_clients);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);
    
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    
    printf("Chat server (multithread) dang mo o cong 9000...\n");
    
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        
        pthread_mutex_lock(&mutex);
        if (num_clients < MAX_CLIENTS) {
            clients[num_clients++] = client;
            printf("Client moi: %d. Tong so: %d\n", client, num_clients);
            
            pthread_t id;
            pthread_create(&id, NULL, client_thread, &client);
            pthread_detach(id);
        } else {
            char *msg = "Server da day!\n";
            send(client, msg, strlen(msg), 0);
            close(client);
        }
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}