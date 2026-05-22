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

void *handleClient(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char buffer[1024];
    char ask_cmd[] =
        "Send command (format: GET_TIME [format], Valid format: dd/mm/yyyy, "
        "dd/mm/yy, mm/dd/yyyy, mm/dd/yy):\n";

    send(client, ask_cmd, strlen(ask_cmd), 0);

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

        char cmd[64];
        char format[64];

        // tach chuoi de lay lenh va dinh dang
        int n = sscanf(buffer, "%s %s", cmd, format);

        // kiem tra lenh
        if (n != 2 || strcmp(cmd, "GET_TIME") != 0) {
            char err_syntax[] = "Invalid command. Use: GET_TIME [format]\n";
            send(client, err_syntax, strlen(err_syntax), 0);
            continue;
        }

        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char time_str[128];

        // kiem tra format client gui va tra ve thoi gian tuong ung
        if (strcmp(format, "dd/mm/yyyy") == 0) {
            strftime(time_str, sizeof(time_str), "%d/%m/%Y\n", t);
            send(client, time_str, strlen(time_str), 0);
        } else if (strcmp(format, "dd/mm/yy") == 0) {
            strftime(time_str, sizeof(time_str), "%d/%m/%y\n", t);
            send(client, time_str, strlen(time_str), 0);
        } else if (strcmp(format, "mm/dd/yyyy") == 0) {
            strftime(time_str, sizeof(time_str), "%m/%d/%Y\n", t);
            send(client, time_str, strlen(time_str), 0);
        } else if (strcmp(format, "mm/dd/yy") == 0) {
            strftime(time_str, sizeof(time_str), "%m/%d/%y\n", t);
            send(client, time_str, strlen(time_str), 0);
        } else {
            // client nhap sai dinh dang
            char err_format[] = "Format invalid.\n";
            send(client, err_format, strlen(err_format), 0);
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