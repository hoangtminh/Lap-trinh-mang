#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int check_login(char *user, char *pass) {
    FILE *f = fopen("db.txt", "r");
    if (!f) return 0;
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) { fclose(f); return 1; }
    }
    fclose(f); return 0;
}

void *telnet_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    int authenticated = 0;
    char buf[256];

    send(client_fd, "User Pass: ", 11, 0);

    while (1) {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = 0;
        while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) buf[--ret] = 0;

        if (!authenticated) {
            char user[50], pass[50];
            if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                authenticated = 1;
                send(client_fd, "Login success. Enter command: ", 30, 0);
            } else {
                send(client_fd, "Login failed. Try again: ", 25, 0);
            }
        } else {
            char cmd[512], out_file[50];
            sprintf(out_file, "out_%d.txt", client_fd);
            sprintf(cmd, "%s > %s 2>&1", buf, out_file);
            system(cmd);
            
            FILE *f = fopen(out_file, "r");
            if (f) {
                char result[2048];
                size_t bytes = fread(result, 1, sizeof(result)-1, f);
                result[bytes] = 0;
                fclose(f);
                send(client_fd, bytes > 0 ? result : "Command executed.\n", bytes > 0 ? bytes : 18, 0);
                remove(out_file);
            }
        }
    }
    close(client_fd);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(8082), htonl(INADDR_ANY)};
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, telnet_handler, new_sock);
        pthread_detach(tid);
    }
    return 0;
}