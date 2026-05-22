#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

void *handleClient(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char buffer[1024];
    char ask_login[] = "Please enter login info (format: username password):\n";

    bool client_check = false;

    // hoi user va pass
    send(client, ask_login, strlen(ask_login), 0);

    while (1) {
        // kiem tra nhan du lieu tu client
        int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            printf("Client number %d disconnected\n", client);
            break;
        }

        buffer[bytes_received] = '\0';

        // bo ky tu '\n' va '\r'
        buffer[strcspn(buffer, "\r\n")] = 0;

        // neu nguoi dung chi go enter thi bo qua
        if (strlen(buffer) == 0)
            continue;

        // kiem tra neu client chua dang nhap thanh cong
        if (!client_check) {
            if (checkLogin(buffer)) {
                client_check = true;

                char success_msg[] = "Login successful. Enter command:\n";
                send(client, success_msg, strlen(success_msg), 0);

                printf("Client %d logged in successfully\n", client);
            } else {
                char error_msg[] =
                    "Login failed. Wrong username or password. Try again\n";
                send(client, error_msg, strlen(error_msg), 0);
            }
        } else {
            printf("Client %d execute: %s\n", client, buffer);

            // moi client su dung 1 file rieng de tranh canh tranh
            char out_filename[64];
            snprintf(out_filename, sizeof(out_filename), "out_%d.txt", client);

            // tao cau lenh: "lenh > out_{client}.txt"
            char sys_cmd[1200];
            snprintf(sys_cmd, sizeof(sys_cmd), "%s > %s", buffer, out_filename);

            // goi ham system thuc thi
            // vi du: ls > out_{client}.txt. he dieu hanh se thuc hien lenh
            // va ghi ket qua vao out_{client}.txt
            system(sys_cmd);

            // doc file out_{client}.txt de tra ve ket qua cho client
            FILE *f_out = fopen(out_filename, "r");

            if (f_out != NULL) {
                char out_buf[1024];
                int bytes_read;

                // vong lap doc den het file de gui tra client
                while ((bytes_read =
                            fread(out_buf, 1, sizeof(out_buf), f_out)) > 0) {
                    send(client, out_buf, bytes_read, 0);
                }

                fclose(f_out);
                remove(out_filename); // xoa file
            } else {
                char err_exec[] = "Loi thuc thi hoac khong co ket qua\n";
                send(client, err_exec, strlen(err_exec), 0);
            }

            char ask_cmd[] = "\nEnter command: ";
            send(client, ask_cmd, strlen(ask_cmd), 0);
        }
    }

    close(client);
    return NULL;
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

    while (1) {
        // neu co yeu cau ket noi
        // chap nhan va tao luong xu ly client
        int client = accept(listen_sock, NULL, NULL);

        if (client < 0) {
            perror("accept() failed");
            continue;
        }

        printf("New client accepted: %d\n", client);

        pthread_t thread_id;

        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client;

        // tao luong rieng de xu ly client
        if (pthread_create(&thread_id, NULL, handleClient, client_ptr) != 0) {
            perror("pthread_create() failed");
            close(client);
            free(client_ptr);
            continue;
        }

        // tach luong de khong can pthread_join
        pthread_detach(thread_id);
    }

    close(listen_sock);
    return 0;
}