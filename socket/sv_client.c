#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client < 0) {
        perror("Khong the tao socket");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(server_ip);
    addr.sin_port = htons(server_port);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Ket noi that bai");
        return 1;
    }

    char mssv[20], hoten[100], ngaysinh[20], diem[10];
    char send_buf[512];

    while (1) {
        printf("\n--- Nhap thong tin sinh vien ---\n");
        
        printf("MSSV: ");
        fgets(mssv, sizeof(mssv), stdin);
        mssv[strcspn(mssv, "\n")] = 0;

        if (strcmp(mssv, "exit") == 0) break;

        printf("Ho ten: ");
        fgets(hoten, sizeof(hoten), stdin);
        hoten[strcspn(hoten, "\n")] = 0;

        printf("Ngay sinh (YYYY-MM-DD): ");
        fgets(ngaysinh, sizeof(ngaysinh), stdin);
        ngaysinh[strcspn(ngaysinh, "\n")] = 0;

        printf("Diem trung binh: ");
        fgets(diem, sizeof(diem), stdin);
        diem[strcspn(diem, "\n")] = 0;

        sprintf(send_buf, "%s %s %s %s", mssv, hoten, ngaysinh, diem);

        send(client, send_buf, strlen(send_buf), 0);
        printf("=> Da gui du lieu.\n");
    }

    close(client);
    printf("Da ngat ket noi.\n");

    return 0;
}