#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>

int waiting_client = -1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ClientPair {
    int client1;
    int client2;
};

void *chat_pair_thread(void *param) {
    struct ClientPair *pair = (struct ClientPair *)param;
    int c1 = pair->client1;
    int c2 = pair->client2;
    free(pair);
    
    char buf1[256], buf2[256];
    fd_set fds;
    int max_fd = (c1 > c2) ? c1 : c2;
    
    printf("Da ghep cap: %d va %d\n", c1, c2);
    
    while (1) {
        FD_ZERO(&fds);
        FD_SET(c1, &fds);
        FD_SET(c2, &fds);
        
        if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) {
            break;
        }
        
        if (FD_ISSET(c1, &fds)) {
            int len = recv(c1, buf1, sizeof(buf1), 0);
            if (len <= 0) break;
            send(c2, buf1, len, 0);
        }
        
        if (FD_ISSET(c2, &fds)) {
            int len = recv(c2, buf2, sizeof(buf2), 0);
            if (len <= 0) break;
            send(c1, buf2, len, 0);
        }
    }
    
    close(c1);
    close(c2);
    printf("Mot client ngat ket noi. Da dong ket noi cap %d va %d.\n", c1, c2);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }
    
    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }
    
    printf("Server chat 2 nguoi dang cho ket noi o cong 8080...\n");
    
    while (1) {
        int client = accept(listener, NULL, NULL);
        if (client == -1) continue;
        printf("Client moi ket noi: %d\n", client);
        
        pthread_mutex_lock(&mutex);
        if (waiting_client == -1) {
            waiting_client = client;
            char *msg = "Ban dang o trong hang doi. Cho nguoi thu 2...\n";
            send(client, msg, strlen(msg), 0);
        } else {
            struct ClientPair *pair = malloc(sizeof(struct ClientPair));
            pair->client1 = waiting_client;
            pair->client2 = client;
            waiting_client = -1;
            
            char *msg = "Da tim thay nguoi ghep cap. Bat dau chat!\n";
            send(pair->client1, msg, strlen(msg), 0);
            send(pair->client2, msg, strlen(msg), 0);
            
            pthread_t id;
            pthread_create(&id, NULL, chat_pair_thread, pair);
            pthread_detach(id);
        }
        pthread_mutex_unlock(&mutex);
    }
    close(listener);
    return 0;
}