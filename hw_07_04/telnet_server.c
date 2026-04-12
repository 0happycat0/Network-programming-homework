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

void removeClient(int *clients, bool *clients_check, int *nClients, int i) {
    // ham xoa phan tu thu i cua mang (khong giu thu tu mang)
    // bang cach: ghi de phan tu thu i bang phan tu cuoi roi giam size di 1
    if (i < *nClients - 1) {
        clients[i] = clients[*nClients - 1];
        clients_check[i] = clients_check[*nClients - 1];
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
    bool clients_check[64] = {0}; // check xem client login thanh cong chua

    char buffer[1024];
    char ask_login[] = "Please enter login info (format: username password):\n";

    while (1) {
        FD_ZERO(&read_fds); // reset fds
        FD_SET(listen_sock,
               &read_fds); // them listen_sock vao danh sach theo doi
        int max_fd = listen_sock;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i], &read_fds); // them client vao danh sach theo doi
            if (clients[i] > max_fd)
                max_fd = clients[i];
        }

        // theo doi cac fd
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
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

            // hoi user va pass
            send(client, ask_login, strlen(ask_login), 0);
        }

        // kiem tra nhan du lieu tu client
        for (int i = 0; i < num_clients; i++) {
            if (FD_ISSET(clients[i], &read_fds)) {
                int bytes_received =
                    recv(clients[i], buffer, sizeof(buffer) - 1, 0);

                if (bytes_received <= 0) {
                    printf("Client number %d disconnected\n", clients[i]);
                    removeClient(clients, clients_check, &num_clients, i);
                    i--;
                    continue;
                }

                buffer[bytes_received] = '\0';
                // bo ky tu '\n' va '\r'
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
                        send(clients[i], success_msg, strlen(success_msg), 0);
                        printf("Client %d logged in successfully\n",
                               clients[i]);
                    } else {
                        char error_msg[] = "Login failed. Wrong username or "
                                           "password. Try again\n";
                        send(clients[i], error_msg, strlen(error_msg), 0);
                    }
                } else {
                    printf("Client %d execute: %s\n", clients[i], buffer);

                    // tao cau lenh: "lenh > out.txt"
                    char sys_cmd[1050];
                    snprintf(sys_cmd, sizeof(sys_cmd), "%s > out.txt", buffer);

                    // goi ham system thuc thi
                    // vi du: ls > out.txt. he dieu hanh se thuc hien lenh va
                    // ghi ket qua vao out.txt
                    system(sys_cmd);

                    // doc file out.txt de tra ve ket qua cho client
                    FILE *f_out = fopen("out.txt", "r");
                    if (f_out != NULL) {
                        char out_buf[1024];
                        int bytes_read;
                        // vong lap doc den het file de gui tra client
                        while ((bytes_read = fread(out_buf, 1, sizeof(out_buf),
                                                   f_out)) > 0) {
                            send(clients[i], out_buf, bytes_read, 0);
                        }
                        fclose(f_out);
                    } else {
                        char err_exec[] =
                            "Loi thuc thi hoac khong co ket qua.\n";
                        send(clients[i], err_exec, strlen(err_exec), 0);
                    }

                    send(clients[i],
                         "\nEnter command: ", sizeof("\nEnter command: "), 0);
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}