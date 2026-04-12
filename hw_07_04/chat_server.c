#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int removeClient(int *clients, bool *clients_check, char clients_name[][128],
                 int *nClients, int i) {
    // ham xoa phan tu thu i cua mang (khong giu thu tu mang)
    // bang cach: ghi de phan tu thu i bang phan tu cuoi roi giam size di 1
    if (i < *nClients - 1) {
        clients[i] = clients[*nClients - 1];
        clients_check[i] = clients_check[*nClients - 1];
        strcpy(clients_name[i], clients_name[*nClients - 1]);
    }
    *nClients -= 1;
}

int main() {
    int port = 8888;

    // khai bao dia chi tai cong 8888
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // tao listen sock
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

    if (listen(listen_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at port %d...\n", port);

    fd_set read_fds;
    int clients[64];              // luu fd cua client
    int num_clients = 0;          // so client
    char clients_name[64][128];   // ten client
    bool clients_check[64] = {0}; // check xem client nhap dung chua

    char buffer[1024];
    char ask_name[] =
        "Please enter client id (format: client_id:client_name)\n\0";

    while (1) {
        FD_ZERO(&read_fds); // reset fds
        FD_SET(listen_sock,
               &read_fds); // them listen_sock vao danh sach theo doi
        int max_fd = listen_sock + 1;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i], &read_fds); // them client vao danh sach theo doi
            if (clients[i] + 1 > max_fd)
                max_fd = clients[i] + 1;
        }

        // theo doi cac fd
        if (select(max_fd, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select() failed");
            break;
        }

        // neu co yeu cau ket noi
        if (FD_ISSET(listen_sock, &read_fds)) {
            // chap nhan va ghi vao mang clients
            int client = accept(listen_sock, NULL, NULL);
            clients[num_clients] = client;
            clients_check[num_clients] = false;
            num_clients++;

            // hoi ten client
            send(client, ask_name, sizeof(ask_name), 0);
        }

        // kiem tra nhan du lieu tu client
        for (int i = 0; i < num_clients; i++) {
            if (FD_ISSET(clients[i], &read_fds)) {
                int bytes_received =
                    recv(clients[i], buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    printf("Client number %d disconnected\n", clients[i]);
                    removeClient(clients, clients_check, clients_name,
                                 &num_clients, i);
                    i--;
                    continue;
                }
                buffer[bytes_received] = '\0';
                // bo ky tu '\n'
                buffer[strcspn(buffer, "\r\n")] = 0;
                printf("Received from client %d: %s\n", clients[i], buffer);

                // kiem tra neu client chua dang nhap thanh cong
                if (!clients_check[i]) {
                    // kiem tra cu phap: "client_id: client_name"
                    if (strncmp(buffer, "client_id:", 10) == 0) {
                        char *name_ptr = buffer + 10;
                        if (*name_ptr == ' ') // bo qua ' '
                            name_ptr++;
                        if (strlen(name_ptr) > 0) {
                            clients_check[i] = true;
                            strcpy(clients_name[i], name_ptr);

                            printf("Client %d registered as '%s'\n", clients[i],
                                   clients_name[i]);
                            char success_msg[] = "Bat dau chat:\n";
                            send(clients[i], success_msg, strlen(success_msg),
                                 0);
                            continue;
                        }
                    }
                    // neu sai cu phap, tiep tuc nhap lai
                    send(clients[i], ask_name, strlen(ask_name), 0);
                } else {
                    // gui du lieu den cac client khac

                    // thoi gian hien tai
                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);
                    char time_str[64];
                    // Dinh dang thoi gian: 2023/05/06 11:00:00PM
                    strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p",
                             t);

                    // tao buffer gui
                    char send_buf[2048];
                    snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n",
                             time_str, clients_name[i], buffer);

                    // gui den cac client khac (ke ca chua dang nhap)
                    for (int j = 0; j < num_clients; j++) {
                        if (j != i) {
                            send(clients[j], send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}