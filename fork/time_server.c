#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 9001

void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void get_formatted_time(char *format, char *output) {
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);

    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(output, 50, "%d/%m/%Y", info);
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(output, 50, "%d/%m/%y", info);
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(output, 50, "%m/%d/%Y", info);
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(output, 50, "%m/%d/%y", info);
    } else {
        strcpy(output, "INVALID_FORMAT");
    }
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
    printf("Time Server đang chạy trên cổng %d sử dụng Multiprocessing...\n", PORT);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0) continue;

        printf("Chấp nhận kết nối mới. Đang tạo tiến trình con...\n");

        if (fork() == 0) {
            close(listener);
            char buf[256];
            while (1) {
                int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) break;

                buf[ret] = '\0';
                while(ret > 0 && (buf[ret-1] == '\n' || buf[ret-1] == '\r')) buf[--ret] = '\0';

                char cmd[20], format[50];
                if (sscanf(buf, "%s %s", cmd, format) == 2 && strcmp(cmd, "GET_TIME") == 0) {
                    char time_result[50];
                    get_formatted_time(format, time_result);
                    
                    if (strcmp(time_result, "INVALID_FORMAT") == 0) {
                        char *err = "ERROR: Dinh dang khong hop le. Cac format ho tro: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy\n";
                        send(client_fd, err, strlen(err), 0);
                    } else {
                        strcat(time_result, "\n");
                        send(client_fd, time_result, strlen(time_result), 0);
                    }
                } else {
                    char *err = "ERROR: Sai cu phap. Dung: GET_TIME [format]\n";
                    send(client_fd, err, strlen(err), 0);
                }
            }
            printf("Tiến trình con xử lý xong, đóng kết nối.\n");
            close(client_fd);
            exit(0);
        }

        close(client_fd);
    }

    return 0;
}