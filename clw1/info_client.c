#include <arpa/inet.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// info client
int main() {
    char *server_ip = "127.0.0.1";
    int server_port = 8888;

    // tao socket
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // chuyen doi dia chi IP
    struct in_addr server_ip_addr;
    if (inet_pton(AF_INET, server_ip, &server_ip_addr) <= 0) {
        perror("IP address is invalid");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr = server_ip_addr;

    // ket noi server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    char buffer[63636];
    int offset = 0;
    char current_working_dir[1024];

    // lay duong dan thu muc hien tai
    getcwd(current_working_dir, sizeof(current_working_dir));

    // ten thu muc hien tai
    char *dir_name = strrchr(current_working_dir, '/');
    dir_name++; // bo '/'

    strcpy(buffer + offset, dir_name);
    offset += strlen(dir_name) + 1;

    // doc danh sach file
    DIR *d;
    struct dirent *dir;
    struct stat file_stat;

    // mo thu muc hien tai
    d = opendir(".");
    if (d != NULL) {
        // duyet thu muc
        while ((dir = readdir(d)) != NULL) {
            // lay thong tin cua muc hien tai
            if (stat(dir->d_name, &file_stat) == 0) {
                // kiem tra xem co phai la file thong thuong khong
                if (S_ISREG(file_stat.st_mode)) {
                    // ghi ten file vao buffer
                    strcpy(buffer + offset, dir->d_name);
                    offset += strlen(dir->d_name) + 1;

                    // ghi kich thuoc file
                    int file_size = (int)file_stat.st_size;
                    int net_size = htonl(file_size);
                    memcpy(buffer + offset, &net_size, sizeof(int));
                    offset += sizeof(int);
                }
            }
            else {
                perror("stat() failed");
                exit(EXIT_FAILURE);
            }
        }
        // dong thu muc
        closedir(d);
    }

    buffer[offset] = '\0';
    offset++;

    // gui du lieu den server
    if (send(sock, buffer, offset, 0) < 0) {
        perror("send() failed");
    }

    close(sock);
    return 0;
}