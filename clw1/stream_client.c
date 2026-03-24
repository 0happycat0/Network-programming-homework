#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// chuong trinh client
int main() {
    char *server_ip = "127.0.0.1";
    int server_port = 8888;

    // tao socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    // ket noi toi server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    printf("Da ket noi toi server. Nhap van ban (nhap 'exit' de thoat):\n");

    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));

        fgets(buffer, sizeof(buffer), stdin);

        // bo ky tu '\n'
        buffer[strcspn(buffer, "\n")] = '\0';

        // nhap exit thi thoat
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        // gui du lieu sang server
        if (send(sock, buffer, strlen(buffer), 0) < 0) {
            perror("send() failed");
            break;
        }
    }

    // dong ket noi
    close(sock);
    return 0;
}