#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// chuong trinh server
int main() {
    int server_port = 8888;

    // tao socket
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_port);

    // gan dia chi
    if (bind(server_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // lang nghe ket noi
    if (listen(server_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at port %d...\n", server_port);

    // chap nhan ket noi
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock =
        accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    printf("Client is connected\n");

    char buffer[1024];
    int bytes_received;

    int total_count = 0; // tong so lan xuat hien cua chuoi
    int match_index = 0; // chi so ky tu tiep theo can khop
    char target[] = "0123456789";
    int target_len = strlen(target);

    // nhan du lieu
    while (1) {
        bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);

        if (bytes_received < 0) {
            perror("recv() failed");
            break;
        } else if (bytes_received == 0) {
            printf("Client is disconnected\n");
            break;
        }

        // duyet qua tung byte nhan duoc trong lan nay
        for (int i = 0; i < bytes_received; i++) {
            // neu ky tu hien tai khop voi ky tu can tim tiep theo
            if (buffer[i] == target[match_index]) {
                match_index++; // tang vi tri can tim len 1

                // neu da khop du 10 ky tu
                if (match_index == target_len) {
                    total_count++;   // tang tong so lan xuat hien
                    match_index = 0; // reset index
                }
            } else {
                // neu khong khop, kiem tra xem no co phai la ky tu bat dau
                // khong
                if (buffer[i] == target[0]) {
                    match_index = 1; // bat dau chuoi moi
                } else {
                    match_index = 0; // xoa trang thai, tim lai tu dau
                }
            }
        }

        // in ra ket qua sau moi lan cap nhat
        printf("Nhan %d bytes. Tong so lan xuat hien xau '0123456789': %d\n",
               bytes_received, total_count);
    }

    // dong socket
    close(client_sock);
    close(server_sock);

    return 0;
}