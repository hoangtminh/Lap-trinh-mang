#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>

#define MAX_CLIENTS 100

typedef struct {
    int fd;
    int authenticated;
    char client_ip[INET_ADDRSTRLEN];
} TelnetClient;

int check_login(char *user, char *pass) {
    FILE *f = fopen("db.txt", "r");
    if (!f) {
        printf("Khong mo duoc file\n");
        return 0;
    }
    char u[50], p[50];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(user, u) == 0 && strcmp(pass, p) == 0) {
            fclose(f); 
            return 1;
        }
    }
    fclose(f); 
    return 0;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8082);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind() failed");
        return 1;
    }
    listen(listener, 5);
    printf("Telnet Server dang chay tren port 8081...\n");

    TelnetClient clients[MAX_CLIENTS];
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
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
            
            clients[nClients].fd = client_fd;
            clients[nClients].authenticated = 0;
            inet_ntop(AF_INET, &client_addr.sin_addr, clients[nClients].client_ip, INET_ADDRSTRLEN);
            
            printf("Client moi tu IP: %s (FD: %d)\n", clients[nClients].client_ip, client_fd);
            
            nClients++;
            send(client_fd, "User Pass: ", 11, 0);
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].fd, &fdread)) {
                char buf[256];
                int ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    printf("Client %s da thoat\n", clients[i].client_ip);
                    close(clients[i].fd);
                    clients[i] = clients[nClients - 1];
                    nClients--; i--; continue;
                }
                
                buf[ret] = 0;
                while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) {
                    buf[--ret] = 0;
                }

                if (!clients[i].authenticated) {
                    char user[50], pass[50];
                    if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                        clients[i].authenticated = 1;
                        printf("Client %s (FD: %d) dang nhap thanh cong voi user: %s\n", clients[i].client_ip, clients[i].fd, user);
                        send(clients[i].fd, "Login success. Enter command: ", 30, 0);
                    } else {
                        printf("Client %s (FD: %d) dang nhap that bai voi chuoi: %s\n", clients[i].client_ip, clients[i].fd, buf);
                        send(clients[i].fd, "Login failed. Try again: ", 25, 0);
                    }
                } else {
                    printf("[COMMAND] Client %s thuc thi: %s\n", clients[i].client_ip, buf);
                    char cmd[300];
                    sprintf(cmd, "%s > out.txt", buf);
                    system(cmd);
                    
                    FILE *f = fopen("out.txt", "r");
                    char result[2048] = {0};
                    if (f) {
                        fread(result, 1, sizeof(result), f);
                        fclose(f);
                        send(clients[i].fd, result, strlen(result), 0);
                    }
                }
            }
        }
    }
    return 0;
}