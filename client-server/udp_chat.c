#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Sử dụng: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("Không thể tạo socket");
        exit(1);
    }

    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);

    unsigned long ul_stdin = 1;
    ioctl(STDIN_FILENO, FIONBIO, &ul_stdin);

    struct sockaddr_in addr_s;
    memset(&addr_s, 0, sizeof(addr_s));
    addr_s.sin_family = AF_INET;
    addr_s.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_s.sin_port = htons(port_s);

    if (bind(sockfd, (struct sockaddr *)&addr_s, sizeof(addr_s)) < 0) {
        perror("Bind thất bại");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in addr_d;
    memset(&addr_d, 0, sizeof(addr_d));
    addr_d.sin_family = AF_INET;
    addr_d.sin_port = htons(port_d);
    addr_d.sin_addr.s_addr = inet_addr(ip_d);

    printf("UDP Chat khởi tạo thành công!\n");
    printf("- Đang nghe tại cổng: %d\n", port_s);
    printf("- Gửi đến: %s:%d\n", ip_d, port_d);
    printf("---------------------------\n");

    char buf_recv[BUFFER_SIZE];
    char buf_send[BUFFER_SIZE];

    while (1) {
        struct sockaddr_in remote_addr;
        socklen_t addr_len = sizeof(remote_addr);
        int ret = recvfrom(sockfd, buf_recv, sizeof(buf_recv) - 1, 0, 
                           (struct sockaddr *)&remote_addr, &addr_len);
        
        if (ret > 0) {
            buf_recv[ret] = '\0';
            printf("Nhận được: %s\n", buf_recv);
        }

        if (fgets(buf_send, sizeof(buf_send), stdin) != NULL) {
            buf_send[strcspn(buf_send, "\n")] = 0;
            
            if (strlen(buf_send) > 0) {
                sendto(sockfd, buf_send, strlen(buf_send), 0, 
                       (struct sockaddr *)&addr_d, sizeof(addr_d));
                
                if (strcmp(buf_send, "exit") == 0) break;
            }
        }

        usleep(10000); 
    }

    close(sockfd);
    return 0;
}