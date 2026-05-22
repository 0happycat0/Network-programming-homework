#include <arpa/inet.h>
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

void signalHandler(int signo) {
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

    signal(SIGCHLD, signalHandler);
    while (1) {
        // neu co yeu cau ket noi
        // chap nhan va ghi vao mang clients
        int client = accept(listen_sock, NULL, NULL);
        printf("New client accepted: %d\n", client);

        if (fork() == 0) {
            close(listen_sock);       // dong listen_sock vi khong dung
            signal(SIGCHLD, SIG_DFL); // tien trinh con khong quan tam SIGCHILD

            char buffer[63686];
            char filepath[1024];
            char temp_file_list[60000] = "";
            DIR *d;
            struct dirent *dir;
            struct stat file_stat;
            int count_file = 0;

            d = opendir("./data");
            if (d != NULL) {
                while ((dir = readdir(d)) != NULL) {
                    // khong xet thu muc hien tai va thu muc cha
                    if (strcmp(dir->d_name, ".") == 0 ||
                        strcmp(dir->d_name, "..") == 0)
                        continue;

                    // ./data/{ten_file}
                    snprintf(filepath, sizeof(filepath), "./data/%s", dir->d_name);
                    if (stat(filepath, &file_stat) == 0) {
                        // kiem tra xem co phai la file thong thuong khong
                        if (S_ISREG(file_stat.st_mode)) {
                            count_file++;
                            strcat(temp_file_list, dir->d_name);
                            strcat(temp_file_list, "\r\n"); // chen \r\n
                        }
                    }
                }
                closedir(d); // dong thu muc
            }

            if (count_file == 0) {
                char *err_msg = "ERROR No files to download \r\n";
                send(client, err_msg, strlen(err_msg), 0);
                close(client);
                exit(0);
            } else {
                char file_info[100];
                // dong dau la "OK N\r\n", tiep theo la ten cac file, ket thuc boi "\r\n"
                sprintf(file_info, "OK %d\r\n%s\r\n", count_file, temp_file_list);
                send(client, file_info, strlen(file_info), 0);
            }

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

                // tao duong dan day du
                snprintf(filepath, sizeof(filepath), "./data/%s", buffer);
                // neu duong dan ton tai va la file 
                if (stat(filepath, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                    char ok_msg[256];
                    sprintf(ok_msg, "OK %ld\r\n", file_stat.st_size); // OK kich_thuoc_file
                    send(client, ok_msg, strlen(ok_msg), 0);

                    // gui noi dung file
                    FILE *fp = fopen(filepath, "rb");
                    if (fp != NULL) {
                        int bytes_read;
                        char file_buf[4096];
                        while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
                            send(client, file_buf, bytes_read, 0);
                        }
                        fclose(fp);
                    }
                    
                    // dong ket noi sau khi gui xong
                    printf("File %s sent to client %d. Closing connection.\n", buffer, client);
                    break; 
                } else {
                    // file khong ton tai thi gui thong bao loi va tiep tuc vong lap doi ten file khac
                    char *err_msg = "ERROR File not found. Send another filename.\r\n";
                    send(client, err_msg, strlen(err_msg), 0);
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