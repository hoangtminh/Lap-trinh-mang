#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define PORT 8082

int check_login(char *user, char *pass) {
    FILE *f = fopen("db.txt", "r");
    if (!f) {
        printf("Khong mo duoc file db.txt\n");
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

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind() failed");
        return 1;
    }
    listen(listener, 10);
    printf("Telnet Server (Multiprocessing) dang chay tren port %d...\n", PORT);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
        return 1;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept() failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client moi ket noi tu IP: %s (FD: %d)\n", client_ip, client_fd);

        if (fork() == 0) {
            close(listener);
            
            send(client_fd, "User Pass: ", 11, 0);
            int authenticated = 0;
            char buf[256];

            while (1) {
                int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) break;

                buf[ret] = 0;
                while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) {
                    buf[--ret] = 0;
                }

                if (!authenticated) {
                    char user[50], pass[50];
                    if (sscanf(buf, "%s %s", user, pass) == 2 && check_login(user, pass)) {
                        authenticated = 1;
                        printf("[%d] Client %s dang nhap thanh cong\n", getpid(), client_ip);
                        send(client_fd, "Login success. Enter command: ", 30, 0);
                    } else {
                        send(client_fd, "Login failed. Try again: ", 25, 0);
                    }
                } else {
                    printf("[%d] COMMAND tu %s: %s\n", getpid(), client_ip, buf);
                    
                    char out_file[64];
                    sprintf(out_file, "out_%d.txt", getpid());
                    
                    char cmd[512];
                    sprintf(cmd, "%s > %s 2>&1", buf, out_file);
                    system(cmd);
                    
                    FILE *f = fopen(out_file, "r");
                    if (f) {
                        char result[2048];
                        size_t bytes_read = fread(result, 1, sizeof(result) - 1, f);
                        fclose(f);
                        remove(out_file);

                        if (bytes_read > 0) {
                            send(client_fd, result, bytes_read, 0);
                        } else {
                            send(client_fd, "Command executed.\n", 18, 0);
                        }
                    }
                }
            }
            printf("Client %s (PID: %d) da thoat\n", client_ip, getpid());
            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    return 0;
}