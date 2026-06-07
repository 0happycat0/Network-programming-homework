#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
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

// ham lay content type dua vao duoi file
char *get_content_type(char *file_path) {
    char *ext = strrchr(file_path, '.');

    if (ext == NULL) {
        return "application/octet-stream";
    }

    // file van ban
    if (strcmp(ext, ".txt") == 0) {
        return "text/plain; charset=UTF-8";
    } else if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) {
        return "text/html; charset=UTF-8";
    } else if (strcmp(ext, ".css") == 0) {
        return "text/css; charset=UTF-8";
    } else if (strcmp(ext, ".js") == 0) {
        return "application/javascript; charset=UTF-8";
    }

    // file anh
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    } else if (strcmp(ext, ".png") == 0) {
        return "image/png";
    } else if (strcmp(ext, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(ext, ".webp") == 0) {
        return "image/webp";
    }

    // file audio
    else if (strcmp(ext, ".mp3") == 0) {
        return "audio/mpeg";
    } else if (strcmp(ext, ".wav") == 0) {
        return "audio/wav";
    } else if (strcmp(ext, ".ogg") == 0) {
        return "audio/ogg";
    }

    // file video
    else if (strcmp(ext, ".mp4") == 0) {
        return "video/mp4";
    } else if (strcmp(ext, ".webm") == 0) {
        return "video/webm";
    } else if (strcmp(ext, ".avi") == 0) {
        return "video/x-msvideo";
    }

    return "application/octet-stream";
}

// ham gui file ve trinh duyet
void send_file(int client, char *file_path) {
    FILE *file = fopen(file_path, "rb");

    if (file == NULL) {
        send_error_page(client, "Không thể mở file.");
        return;
    }

    // lay kich thuoc file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char header[1024];

    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            get_content_type(file_path), file_size);

    send(client, header, strlen(header), 0);

    // doc file va gui tung phan ve trinh duyet
    char buffer[4096];
    int bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client, buffer, bytes_read, 0);
    }

    fclose(file);
}

// ham doi ky tu dac biet trong URL
void url_decode(char *src, char *dest) {
    char a, b;

    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && isxdigit(a) &&
            isxdigit(b)) {
            if (a >= 'a') {
                a -= 'a' - 'A';
            }
            if (a >= 'A') {
                a = a - 'A' + 10;
            } else {
                a -= '0';
            }

            if (b >= 'a') {
                b -= 'a' - 'A';
            }
            if (b >= 'A') {
                b = b - 'A' + 10;
            } else {
                b -= '0';
            }

            *dest++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dest++ = ' ';
            src++;
        } else {
            *dest++ = *src++;
        }
    }

    *dest = '\0';
}

// ham gui danh sach thu muc va file ve trinh duyet
void send_directory_page(int client, char *dir_path, char *url_path) {
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        send_error_page(client, "Không thể mở thư mục.");
        return;
    }

    char html[65536];

    sprintf(html,
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset='UTF-8'>"
            "<title>HTTP File Server</title>"
            "</head>"
            "<body>"
            "<h2>Danh sách thư mục: %s</h2>"
            "<ul>",
            url_path);

    // neu khong phai thu muc goc thi them link quay lai
    if (strcmp(url_path, "/") != 0) {
        strcat(html, "<li><a href='../'><b>..</b></a></li>");
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // bo qua thu muc hien tai va thu muc cha
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char item_path[2048];
        sprintf(item_path, "%s/%s", dir_path, entry->d_name);

        struct stat st;

        if (stat(item_path, &st) < 0) {
            continue;
        }

        char href[2048];

        if (strcmp(url_path, "/") == 0) {
            sprintf(href, "/%s", entry->d_name);
        } else {
            sprintf(href, "%s/%s", url_path, entry->d_name);
        }

        // neu la thu muc thi in dam
        if (S_ISDIR(st.st_mode)) {
            strcat(html, "<li>");
            strcat(html, "<a href='");
            strcat(html, href);
            strcat(html, "/'><b>");
            strcat(html, entry->d_name);
            strcat(html, "/</b></a>");
            strcat(html, "</li>");
        }

        // neu la file thi in nghieng
        else if (S_ISREG(st.st_mode)) {
            strcat(html, "<li>");
            strcat(html, "<a href='");
            strcat(html, href);
            strcat(html, "'><i>");
            strcat(html, entry->d_name);
            strcat(html, "</i></a>");
            strcat(html, "</li>");
        }
    }

    strcat(html, "</ul>"
                 "</body>"
                 "</html>");

    closedir(dir);

    send_response(client, html);
}

// ham xu ly request tu client
void handle_request(int client, char *path) {
    char decoded_path[2048];

    // doi url encoding thanh ky tu binh thuong
    url_decode(path, decoded_path);

    // chan truy cap ra ngoai thu muc hien tai
    if (strstr(decoded_path, "..") != NULL) {
        send_error_page(client, "Đường dẫn không hợp lệ.");
        return;
    }

    char file_path[2048];

    // neu request thu muc goc
    if (strcmp(decoded_path, "/") == 0) {
        strcpy(file_path, ".");
    } else {
        sprintf(file_path, ".%s", decoded_path);
    }

    struct stat st;

    if (stat(file_path, &st) < 0) {
        send_error_page(client, "Không tìm thấy file hoặc thư mục.");
        return;
    }

    // neu la thu muc thi tra ve danh sach file va thu muc con
    if (S_ISDIR(st.st_mode)) {
        send_directory_page(client, file_path, decoded_path);
    }

    // neu la file thi tra ve noi dung file
    else if (S_ISREG(st.st_mode)) {
        send_file(client, file_path);
    }

    // cac truong hop khac
    else {
        send_error_page(client, "Không hỗ trợ loại tài nguyên này.");
    }
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

            // file server chi xu ly GET
            if (strcmp(method, "GET") == 0) {
                handle_request(client, path);
            }

            close(client);
            exit(0);
        }

        close(client); // dong client vi tien trinh cha khong dung den
    }

    close(listen_sock);
    return 0;
}