#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

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
    printf("Telnet Server dang chay tren port 8082...\n");

    TelnetClient clients[MAX_CLIENTS];
    struct pollfd fds[MAX_CLIENTS + 1];
    int nClients = 0;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    while (1) {
        for (int i = 0; i < nClients; i++) {
            fds[i + 1].fd = clients[i].fd;
            fds[i + 1].events = POLLIN;
        }

        int poll_count = poll(fds, nClients + 1, -1);
        if (poll_count < 0) {
            perror("poll() failed");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
            
            if (nClients < MAX_CLIENTS) {
                clients[nClients].fd = client_fd;
                clients[nClients].authenticated = 0;
                inet_ntop(AF_INET, &client_addr.sin_addr, clients[nClients].client_ip, INET_ADDRSTRLEN);
                
                printf("Client moi tu IP: %s (FD: %d)\n", clients[nClients].client_ip, client_fd);
                
                nClients++;
                send(client_fd, "User Pass: ", 11, 0);
            } else {
                printf("Server day, tu choi ket noi tu FD: %d\n", client_fd);
                close(client_fd);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (fds[i + 1].revents & POLLIN) {
                char buf[256];
                int ret = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

                if (ret <= 0) {
                    printf("Client %s (FD: %d) da thoat\n", clients[i].client_ip, clients[i].fd);
                    close(clients[i].fd);
                    clients[i] = clients[nClients - 1];
                    nClients--;
                    continue;
                }
                
                buf[ret] = 0;
                while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) {
                    buf[--ret] = 0;
                }
                if (!clients[i].authenticated) {
                    char user[50], pass[50];
                    if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                        clients[i].authenticated = 1;
                        printf("Client %s dang nhap thanh cong: %s\n", clients[i].client_ip, user);
                        send(clients[i].fd, "Login success. Enter command: ", 30, 0);
                    } else {
                        printf("Client %s dang nhap that bai: %s\n", clients[i].client_ip, buf);
                        send(clients[i].fd, "Login failed. Try again: ", 25, 0);
                    }
                } 
                // Nếu đã đăng nhập: Thực thi lệnh hệ thống
                else {
                    printf("[COMMAND] Client %s: %s\n", clients[i].client_ip, buf);
                    char cmd[512];
                    sprintf(cmd, "%s > out_%d.txt", buf, clients[i].fd);
                    system(cmd);
                    
                    FILE *f = fopen(cmd + strlen(buf) + 3, "r");
                    if (f) {
                        char result[2048];
                        size_t bytes_read = fread(result, 1, sizeof(result) - 1, f);
                        result[bytes_read] = 0;
                        fclose(f);
                        if (bytes_read > 0) {
                            send(clients[i].fd, result, bytes_read, 0);
                        } else {
                            send(clients[i].fd, "Command executed with no output.\n", 33, 0);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}