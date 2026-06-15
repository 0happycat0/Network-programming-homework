#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    char *server_host = "lebavui.io.vn";
    int control_port = 21;
    char buffer[2048];

    // phan giai ten mien thanh IP
    struct hostent *he = gethostbyname(server_host);
    if (he == NULL) {
        perror("gethostbyname() failed");
        return 1;
    }
    char *server_ip = inet_ntoa(*(struct in_addr *)he->h_addr_list[0]);
    printf("IP cua server: %s\n", server_ip);

    // ket noi kenh control port 21
    int control_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(control_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(control_sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("connect() port 21 failed");
        return 1;
    }

    // doc xau chao cua FTP server
    int n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // gui username
    send(control_sock, "USER user_20225241\r\n",
         strlen("USER user_20225241\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // gui password
    send(control_sock, "PASS 524111\r\n", strlen("PASS 524111\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // chuyen sang che do bi dong (PASV) de mo kenh du lieu
    send(control_sock, "PASV\r\n", strlen("PASV\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // phan tich thong tin ip va port tu phan hoi cua lenh PASV
    int h1, h2, h3, h4, p1, p2;
    char *pasv_ptr = strchr(buffer, '(');
    sscanf(pasv_ptr, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    int data_port = p1 * 256 + p2; // tinh data port

    // mo socket data va ket noi den data_port
    int data_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in data_addr = {0};
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    data_addr.sin_addr.s_addr = inet_addr(server_ip);
    connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));

    // gui lenh LIST qua socket control
    send(control_sock, "LIST\r\n", strlen("LIST\r\n"), 0);

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // nhan du lieu danh sach file tu socket data
    char list_buf[4096] = {0};
    int total_list_bytes = 0;
    while ((n = recv(data_sock, list_buf + total_list_bytes,
                     sizeof(list_buf) - total_list_bytes - 1, 0)) > 0) {
        total_list_bytes += n;
    }
    list_buf[total_list_bytes] = '\0';
    close(data_sock); // dong socket data sau khi nhan xong danh sach

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // tim file question
    char *q_ptr = strstr(list_buf, "question_");
    if (q_ptr == NULL) {
        printf("Khong tim thay file question_xxxxxx.txt tren server.\n");
        close(control_sock);
        return 1;
    }
    char question_file[128];
    sscanf(
        q_ptr, "%s",
        question_file); // sscanf tu dong cat chuoi tai khoang trang/xuong dong
    printf("File question can tai ve: '%s'\n", question_file);

    // gui lai lenh PASV de lay kenh data moi cho viec tai file
    send(control_sock, "PASV\r\n", strlen("PASV\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';

    pasv_ptr = strchr(buffer, '(');
    sscanf(pasv_ptr, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    data_port = p1 * 256 + p2;

    // ket noi data port
    data_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    data_addr.sin_port = htons(data_port);
    connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));

    // gui lenh RETR de lay noi dung file question
    char retr_cmd[256];
    snprintf(retr_cmd, sizeof(retr_cmd), "RETR %s\r\n", question_file);
    send(control_sock, retr_cmd, strlen(retr_cmd), 0);

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // nhan noi dung file
    char file_content[1024] = {0};
    int total_content_bytes = 0;
    while ((n = recv(data_sock, file_content + total_content_bytes,
                     sizeof(file_content) - total_content_bytes - 1, 0)) > 0) {
        total_content_bytes += n;
    }
    file_content[total_content_bytes] = '\0';
    close(data_sock);

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // loai bo ky tu '\r' hoac '\n' trong noi dung
    file_content[strcspn(file_content, "\r\n")] = 0;
    printf("Noi dung file question: %s\n", file_content);

    // tao ten file answer_xxxxxx.txt ung voi question_xxxxxx.txt
    char answer_file[128];
    char *rand_part =
        strchr(question_file, '_');
    snprintf(answer_file, sizeof(answer_file), "answer%s", rand_part);
    printf("File answer se duoc tao ra: '%s'\n", answer_file);

    // dao nguoc xau ky tu
    int content_len = strlen(file_content);
    char reversed_content[1024];
    for (int idx = 0; idx < content_len; idx++) {
        reversed_content[idx] = file_content[content_len - 1 - idx];
    }
    reversed_content[content_len] = '\0';

    // luu file answer
    FILE *f = fopen(answer_file, "w");
    fprintf(f, "%s", reversed_content);
    fclose(f);
    printf("Da luu file cuc bo voi noi dung dao nguoc: %s\n", reversed_content);

    // goi PASV de lay kenh truyen du lieu cho viec upload
    send(control_sock, "PASV\r\n", strlen("PASV\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';

    pasv_ptr = strchr(buffer, '(');
    sscanf(pasv_ptr, "(%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    data_port = p1 * 256 + p2;

    // ket noi data port
    data_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    data_addr.sin_port = htons(data_port);
    connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr));

    // gui lenh STOR qua control socket
    char stor_cmd[256];
    snprintf(stor_cmd, sizeof(stor_cmd), "STOR %s\r\n", answer_file);
    send(control_sock, stor_cmd, strlen(stor_cmd), 0);

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // day du lieu tu bien vao socket data de thuc hien upload
    send(data_sock, reversed_content, strlen(reversed_content), 0);
    close(data_sock);

    // doc phan hoi server
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    // dong ket noi
    send(control_sock, "QUIT\r\n", strlen("QUIT\r\n"), 0);
    n = recv(control_sock, buffer, sizeof(buffer) - 1, 0);
    buffer[n] = '\0';
    printf("%s", buffer);

    close(control_sock);
    return 0;
}