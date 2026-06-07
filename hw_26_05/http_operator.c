#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void signalHandler(int signo) { // Xử lý sự kiện tiến trình con kết thúc
    printf("signo = %d\n", signo);

    while (waitpid(-1, NULL, WNOHANG) > 0) {
        printf("A child process terminated.\n");
    }

    return;
}

// ham gui response html ve trinh duyet
void send_response(int client, char *html) {
    char response[8192];

    sprintf(response,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(html), html);

    send(client, response, strlen(response), 0);
}

// ham gui trang loi ve trinh duyet
void send_error_page(int client, char *message) {
    char html[4096];

    sprintf(html,
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset='UTF-8'>"
            "<title>Lỗi</title>"
            "</head>"
            "<body>"
            "<h2>Lỗi</h2>"
            "<p>%s</p>"
            "</body>"
            "</html>",
            message);

    send_response(client, html);
}

// ham lay gia tri cua tham so trong query string hoac body
void get_param(char *data, char *key, char *value) {
    value[0] = '\0';

    char temp[1024];
    strcpy(temp, data);

    char *token = strtok(temp, "&");

    while (token != NULL) {
        char param_key[64];
        char param_value[64];

        int n = sscanf(token, "%[^=]=%s", param_key, param_value);

        if (n == 2 && strcmp(param_key, key) == 0) {
            strcpy(value, param_value);
            return;
        }

        token = strtok(NULL, "&");
    }
}

// ham xu ly phep tinh va gui ket qua ve trinh duyet
void handle_calculate(int client, char *data) {
    char a_str[64];
    char b_str[64];
    char op[64];

    get_param(data, "a", a_str);
    get_param(data, "b", b_str);
    get_param(data, "op", op);

    // kiem tra du tham so chua
    if (strlen(a_str) == 0 || strlen(b_str) == 0 || strlen(op) == 0) {
        send_error_page(client, "Thiếu tham số. Ví dụ: "
                                "/?a=10&b=5&op=add");
        return;
    }

    double a = atof(a_str);
    double b = atof(b_str);
    double result = 0;

    char op_symbol[8];

    // kiem tra toan tu va tinh ket qua
    if (strcmp(op, "add") == 0) {
        result = a + b;
        strcpy(op_symbol, "+");
    } else if (strcmp(op, "sub") == 0) {
        result = a - b;
        strcpy(op_symbol, "-");
    } else if (strcmp(op, "mul") == 0) {
        result = a * b;
        strcpy(op_symbol, "*");
    } else if (strcmp(op, "div") == 0) {
        if (b == 0) {
            send_error_page(client, "Không thể chia cho 0");
            return;
        }

        result = a / b;
        strcpy(op_symbol, "/");
    } else {
        send_error_page(client,
                        "Toán tử không hợp lệ. Toán tử hợp lệ: add, sub, "
                        "mul, div.");
        return;
    }

    char html[8192];

    // tao trang html hien thi ket qua
    sprintf(html,
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset='UTF-8'>"
            "<title>Kết quả phép tính</title>"
            "</head>"
            "<body>"
            "<h2>Kết quả phép tính</h2>"
            "<p>Phép tính: %.2lf %s %.2lf</p>"
            "<h3>Kết quả: %.2lf</h3>"
            "</body>"
            "</html>",
            a, op_symbol, b, result);

    send_response(client, html);
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

    signal(SIGCHLD, signalHandler);

    while (1) {
        int client = accept(listen_sock, NULL, NULL);

        if (client < 0) {
            perror("accept() failed");
            continue;
        }

        printf("New client accepted: %d\n", client);

        if (fork() == 0) {
            close(listen_sock);       // dong listen_sock vi khong dung
            signal(SIGCHLD, SIG_DFL); // tien trinh con khong quan tam SIGCHLD

            char buffer[4096];

            // nhan HTTP request tu client
            int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received <= 0) {
                printf("Client number %d disconnected\n", client);
                close(client);
                exit(0);
            }

            buffer[bytes_received] = '\0';

            printf("Request from client:\n%s\n", buffer);

            char method[16];
            char path[1024];

            // lay method va path tu dong dau tien cua HTTP request
            sscanf(buffer, "%s %s", method, path);

            // neu request la GET
            if (strcmp(method, "GET") == 0) {
                char *query = strchr(path, '?');

                // GET phai co tham so nam sau dau ?
                if (query != NULL) {
                    query++; // bo qua ky tu '?'
                    handle_calculate(client, query);
                } else {
                    send_error_page(client, "GET request thiếu tham số. Ví dụ: "
                                            "/?a=10&b=5&op=add");
                }
            }

            // neu request la POST
            else if (strcmp(method, "POST") == 0) {
                char *body = strstr(buffer, "\r\n\r\n");

                if (body != NULL && strlen(body + 4) > 0) {
                    body += 4; // bo qua "\r\n\r\n"
                    handle_calculate(client, body);
                } else {
                    send_error_page(client,
                                    "POST request thiếu body. Ví dụ body: "
                                    "a=10&b=5&op=add");
                }
            }

            close(client);
            exit(0);
        }

        close(client); // dong client vi tien trinh cha khong dung den
    }

    close(listen_sock);
    return 0;
}