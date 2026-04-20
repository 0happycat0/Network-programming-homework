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

// ham kiem tra tai khoan mat khau trong file txt
bool checkLogin(char *input) {
    FILE *f = fopen("database.txt", "r");
    if (f == NULL) {
        printf("Loi: Khong the mo file database.txt\n");
        return false;
    }

    char in_user[64], in_pass[64];
    // tach input ("username password")
    if (sscanf(input, "%s %s", in_user, in_pass) != 2) {
        fclose(f);
        return false;
    }

    char file_user[64], file_pass[64];
    char line[256];
    // doc tung dong cua file de so sanh
    while (fgets(line, sizeof(line), f)) {
        // lay username va password trong file
        if (sscanf(line, "%s %s", file_user, file_pass) == 2) {
            if (strcmp(in_user, file_user) == 0 &&
                strcmp(in_pass, file_pass) == 0) {
                fclose(f);
                return true;
            }
        }
    }

    fclose(f);
    return false;
}

void removeClient(struct pollfd *fds, bool *clients_check, int *num_fds,
                  int i) {
    // ham xoa phan tu thu i cua mang (khong giu thu tu mang)
    // bang cach: ghi de phan tu thu i bang phan tu cuoi roi giam size di 1
    if (i < *num_fds - 1) {
        fds[i] = fds[*num_fds - 1];
        clients_check[i] = clients_check[*num_fds - 1];
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
    bool clients_check[64] = {0}; // check xem client login thanh cong chua

    fds[0].fd = listen_sock; // dua listen_sock vao danh sach theo doi
    fds[0].events = POLLIN;

    char buffer[1024];
    char ask_login[] = "Please enter login info (format: username password):\n";

    while (1) {
        // theo doi cac fd voi thoi gian cho vo han 
        if (poll(fds, num_fds, -1) < 0) {
            perror("poll() failed");
            break;
        }

        // neu co yeu cau ket noi
        if (fds[0].revents & POLLIN) {
            // chap nhan va cap nhat mang fds
            int client = accept(listen_sock, NULL, NULL);

            if (num_fds < max_client) {
                printf("New client connected: %d\n", client);
                fds[num_fds].fd = client;
                fds[num_fds].events = POLLIN;
                clients_check[num_fds] = false;
                num_fds++;

                // hoi user va pass
                send(client, ask_login, strlen(ask_login), 0);
            } else {
                close(client);
            }
        }

        // kiem tra nhan du lieu tu client 
        for (int i = 1; i < num_fds; i++) {
            if (fds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
                int bytes_received =
                    recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_received <= 0) {
                    printf("Client number %d disconnected\n", fds[i].fd);
                    removeClient(fds, clients_check, &num_fds, i);
                    i--;
                    continue;
                }

                buffer[bytes_received] = '\0';
                // bo ky tu '\n'
                buffer[strcspn(buffer, "\r\n")] = 0;

                // neu nguoi dung chi go enter thi bo qua
                if (strlen(buffer) == 0)
                    continue;

                // kiem tra neu client chua dang nhap thanh cong
                if (!clients_check[i]) {
                    if (checkLogin(buffer)) {
                        clients_check[i] = true;
                        char success_msg[] =
                            "Login successful. Enter command:\n";
                        send(fds[i].fd, success_msg, strlen(success_msg), 0);
                        printf("Client %d logged in successfully\n", fds[i].fd);
                    } else {
                        char error_msg[] = "Login failed. Wrong username or "
                                           "password. Try again\n";
                        send(fds[i].fd, error_msg, strlen(error_msg), 0);
                    }
                } else {
                    printf("Client %d execute: %s\n", fds[i].fd, buffer);

                    // tao cau lenh: "lenh > out.txt"
                    char sys_cmd[1050];
                    snprintf(sys_cmd, sizeof(sys_cmd), "%s > out.txt", buffer);

                    // goi ham system thuc thi
                    system(sys_cmd);

                    // doc file out.txt de tra ve ket qua cho client
                    FILE *f_out = fopen("out.txt", "r");
                    if (f_out != NULL) {
                        char out_buf[1024];
                        int bytes_read;
                        while ((bytes_read = fread(out_buf, 1, sizeof(out_buf),
                                                   f_out)) > 0) {
                            send(fds[i].fd, out_buf, bytes_read, 0);
                        }
                        fclose(f_out);
                    } else {
                        char err_exec[] =
                            "Loi thuc thi hoac khong co ket qua.\n";
                        send(fds[i].fd, err_exec, strlen(err_exec), 0);
                    }

                    char prompt[] = "\nEnter command: ";
                    send(fds[i].fd, prompt, strlen(prompt), 0);
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}