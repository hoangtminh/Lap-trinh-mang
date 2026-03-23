#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    int port = atoi(argv[1]);
    char *hello_file = argv[2];
    char *write_file = argv[3];

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        perror("setsockopt failed"); 
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    listen(listener, 5)
    printf("Server running at %d port\n", port);

    int client = accept(listener, NULL, NULL);
    if (client < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    FILE *f_hello = fopen(hello_file, "r");
    if (f_hello != NULL) {
        char hello_msg[256];
        if (fgets(hello_msg, sizeof(hello_msg), f_hello) != NULL) {
            send(client, hello_msg, strlen(hello_msg), 0);
        }
        fclose(f_hello);
    }

    FILE *f_ghi = fopen(write_file, "a");
    if (f_ghi == NULL) {
        perror("Khong the mo file");
        return 1;
    }

    char buf[256];
    while (1) {
        int ret = recv(client, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            printf("Client da ngat ket noi.\n");
            break;
        }

        buf[ret] = 0;
        printf("Client message: %s\n", buf);

        fprintf(f_ghi, "Client: %s\n", buf);
        fflush(f_ghi);
    }

    fclose(f_ghi);
    close(client);
    close(listener);

    return 0;
}