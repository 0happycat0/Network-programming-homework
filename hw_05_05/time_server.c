#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void signalHandler(int signo) { // Xử lý sự kiện tiến trình con kết thúc
    printf("signo = %d\n", signo);
    int pid = wait(NULL);
    printf("child %d terminated.\n", pid);
    return;
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

    char buffer[1024];
    char ask_cmd[] = "Send command (format: GET_TIME [format], Valid format: dd/mm/yyyy, dd/mm/yy, mm/dd/yyyy, mm/dd/yy):\n";

    signal(SIGCHLD, signalHandler);
    while (1) {
        // neu co yeu cau ket noi
        // chap nhan va ghi vao mang clients
        int client = accept(listen_sock, NULL, NULL);
        printf("New client accepted: %d\n", client);

        if (fork() == 0) {
            close(listen_sock); // dong listen_sock vi khong dung
            signal(SIGCHLD, SIG_DFL); // tien trinh con khong quan tam SIGCHILD

            send(client, ask_cmd, strlen(ask_cmd), 0);

            while (1) {
                // kiem tra nhan du lieu tu client
                int bytes_received =
                    recv(client, buffer, sizeof(buffer) - 1, 0);

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
                } 
                else if (strcmp(format, "dd/mm/yy") == 0) {
                    strftime(time_str, sizeof(time_str), "%d/%m/%y\n", t);
                    send(client, time_str, strlen(time_str), 0);
                } 
                else if (strcmp(format, "mm/dd/yyyy") == 0) {
                    strftime(time_str, sizeof(time_str), "%m/%d/%Y\n", t);
                    send(client, time_str, strlen(time_str), 0);
                } 
                else if (strcmp(format, "mm/dd/yy") == 0) {
                    strftime(time_str, sizeof(time_str), "%m/%d/%y\n", t);
                    send(client, time_str, strlen(time_str), 0);
                } 
                else {
                    // client nhap sai dinh dang
                    char err_format[] = "Format invalid.\n";
                    send(client, err_format, strlen(err_format), 0);
                }
            }
            close(client);
            exit(0);
        }
        close(client); // dong client vi khong dung den
    }

    close(listen_sock);
    return 0;
}