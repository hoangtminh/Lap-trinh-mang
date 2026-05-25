#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void *time_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    char buf[256];

    while (1) {
        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = 0;
        while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) buf[--ret] = 0;

        char cmd[20], format[50], output[50];
        if (sscanf(buf, "%s %s", cmd, format) == 2 && strcmp(cmd, "GET_TIME") == 0) {
            time_t rawtime; struct tm *info;
            time(&rawtime); info = localtime(&rawtime);

            if (strcmp(format, "dd/mm/yyyy") == 0) strftime(output, 50, "%d/%m/%Y\n", info);
            else if (strcmp(format, "dd/mm/yy") == 0) strftime(output, 50, "%d/%m/%y\n", info);
            else if (strcmp(format, "mm/dd/yyyy") == 0) strftime(output, 50, "%m/%d/%Y\n", info);
            else if (strcmp(format, "mm/dd/yy") == 0) strftime(output, 50, "%m/%d/%y\n", info);
            else strcpy(output, "ERROR: Invalid format\n");
            
            send(client_fd, output, strlen(output), 0);
        } else {
            char *err = "ERROR: Use GET_TIME [format]\n";
            send(client_fd, err, strlen(err), 0);
        }
    }
    close(client_fd);
    return NULL;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {AF_INET, htons(9001), htonl(INADDR_ANY)};
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 10);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, time_handler, new_sock);
        pthread_detach(tid);
    }
    return 0;
}