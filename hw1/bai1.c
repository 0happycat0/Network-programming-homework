#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TCP client
int main(int argc, char *argv[]) {
    char *cmd = argv[1];
    char *server_ip = argv[2];
    int server_port = atoi(argv[3]);

    // printf("cmd=%s\n", cmd);
    // printf("ip_inp=%s\n", server_ip);
    // printf("port=%d\n", server_port);

    // kiem tra lenh
    if (strcmp(cmd, "tcp_client") != 0 || argc != 4) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    // tao socket
    int sock;
    char message[1024];
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // chuyen doi dia chi IP
    struct in_addr server_ip_addr;
    if (inet_pton(AF_INET, server_ip, &server_ip_addr) <= 0) {
        perror("IP address is invalid");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr = server_ip_addr;

    // ket noi server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    // gui du lieu tu ban phim
    printf("Nhap du lieu: ");
    fgets(message, sizeof(message), stdin);
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("send() failed");
        exit(EXIT_FAILURE);
    }

    close(sock);
    return 0;
}