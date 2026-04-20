#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int removeClient(struct pollfd *fds, bool *clients_check,
                 char clients_name[][128], int *num_fds, int i) {
    // ham xoa phan tu thu i cua mang (khong giu thu tu mang)
    // bang cach: ghi de phan tu thu i bang phan tu cuoi roi giam size di 1
    if (i < *num_fds - 1) {
        fds[i] = fds[*num_fds - 1];
        clients_check[i] = clients_check[*num_fds - 1];
        strcpy(clients_name[i], clients_name[*num_fds - 1]);
    }
    *num_fds -= 1;
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

    const int max_client = 63;
    struct pollfd fds[64]; // luu fd cua client (mang cac client can theo doi)
    int num_fds = 1;       // so fd
    char clients_name[64][128];   // ten client
    bool clients_check[64] = {0}; // check xem client nhap dung chua

    fds[0].fd = listen_sock; // dua listen_sock vao danh sach theo doi
    fds[0].events = POLLIN;

    char buffer[1024];
    char ask_name[] =
        "Please enter client id (format: client_id:client_name)\n\0";

    while (1) {
        // theo doi cac fd voi thoi gian cho vo han
        if (poll(fds, num_fds, -1) < 0) {
            perror("poll() failed");
            break;
        }

        // neu co yeu cau ket noi
        if (fds[0].revents & POLLIN) { // vi day la bit mask, moi bit tuong ung
                                       // 1 sk xay ra nen ko dung == duoc
            // chap nhan va cap nhat fds
            int client = accept(listen_sock, NULL, NULL);

            if (num_fds < max_client) {
                printf("New client connected: %d\n", client);
                fds[num_fds].fd = client;
                fds[num_fds].events = POLLIN;
                clients_check[num_fds] = false;
                num_fds++;

                // hoi ten client
                send(client, ask_name, sizeof(ask_name), 0);
            } else {
                close(client);
            }
        }

        // kiem tra nhan du lieu tu client
        for (int i = 1; i < num_fds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                int bytes_received = recv(fds[i].fd, buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    printf("Client number %d disconnected\n", fds[i].fd);
                    removeClient(fds, clients_check, clients_name, &num_fds, i);
                    i--;
                    continue;
                }
                buffer[bytes_received] = '\0';
                // bo ky tu '\n'
                buffer[strcspn(buffer, "\r\n")] = 0;
                printf("Received from client %d: %s\n", fds[i].fd, buffer);

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

                            printf("Client %d registered as '%s'\n", fds[i].fd,
                                   clients_name[i]);
                            char success_msg[] = "Bat dau chat:\n";
                            send(fds[i].fd, success_msg, strlen(success_msg),
                                 0);
                            continue;
                        }
                    }
                    // neu sai cu phap, tiep tuc nhap lai
                    send(fds[i].fd, ask_name, strlen(ask_name), 0);
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
                    for (int j = 1; j < num_fds; j++) {
                        if (j != i) {
                            send(fds[j].fd, send_buf, strlen(send_buf), 0);
                        }
                    }
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}