#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// TCP server
int main(int argc, char *argv[]) {

    char *cmd = argv[1];
    int port = atoi(argv[2]);
    char *file_chao = argv[3];
    char *file_luu = argv[4];

    // kiem tra lenh
    if (strcmp(cmd, "tcp_server") != 0 || argc != 5) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    // doc cau chao tu tep tin
    char greeting[1024] = {0};
    FILE *f_chao = fopen(file_chao, "r");
    if (f_chao == NULL) {
        perror("Loi mo tep tin chao");
        exit(EXIT_FAILURE);
    }
    // doc noi dung tep tin (toi da 1023 byte)
    size_t greeting_len = fread(greeting, 1, sizeof(greeting) - 1, f_chao);
    fclose(f_chao);

    // tao socket lang nghe
    int listen_sock;
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr =
        htonl(INADDR_ANY); // lang nghe tren moi IP cua may

    int opt = 1;
    // Thiết lập SO_REUSEADDR để giải phóng cổng ngay khi server tắt
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
        0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // gan dia chi cho socket
    if (bind(listen_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // lang nghe ket noi (5: so luong ket noi toi da)
    if (listen(listen_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at port %d...\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // chap nhan ket noi
    int client_sock =
        accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    // in ra thong tin IP cua client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("Co client ket noi den tu IP: %s\n", client_ip);

    // gui cau chao doc tu tep tin cho client
    if (send(client_sock, greeting, greeting_len, 0) < 0) {
        perror("send() failed");
    }

    // mo tep tin luu du lieu (a: append mode)
    FILE *f_luu = fopen(file_luu, "a");
    if (f_luu == NULL) {
        perror("Loi mo tep tin luu du lieu");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    // nhan du lieu tu client va ghi vao tep tin
    char buffer[1024];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);

    while (bytes_received > 0) {
        fwrite(buffer, 1, bytes_received, f_luu);

        bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
    }

    if (bytes_received <= 0) {
        printf("Disconnected\n");
    }

    printf("Da ghi xong du lieu vao '%s'. Dong ket noi voi client.\n\n",
           file_luu);

    // dong ket noi va dong file cua client do
    fclose(f_luu);
    close(client_sock);

    // dong socket lang nghe
    close(listen_sock);
    return 0;
}