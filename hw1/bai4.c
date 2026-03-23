#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Dinh nghia lai cau truc sinh vien GIONG HET ben sv_client
// De dam bao Server doc dung vung nho ma Client da dong goi
typedef struct {
    char mssv[16];
    char ho_ten[64];
    char ngay_sinh[16];
    float gpa;
} SinhVien;

// TCP server
int main(int argc, char *argv[]) {
    char *cmd = argv[1];
    int port = atoi(argv[2]);
    char *log_file = argv[3];

    // kiem tra lenh
    if (strcmp(cmd, "sv_server") != 0 || argc != 4) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    // tao socket
    int listen_sock;
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // gan dia chi cho socket
    if (bind(listen_sock, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // lang nghe ket noi
    if (listen(listen_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at port %d. Data would be saved at '%s'...\n",
           port, log_file);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // chap nhan ket noi
    int client_sock =
        accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    // Lay IP cua client
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    // lay thoi gian hien tai cua he thong
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];
    // dinh dang thoi gian theo chuan: YYYY-MM-DD HH:MM:SS
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    SinhVien sv;
    memset(&sv, 0, sizeof(SinhVien));

    // nhan truc tiep du lieu vao vung nho cua struct SinhVien
    int bytes_received = recv(client_sock, &sv, sizeof(SinhVien), 0);

    if (bytes_received > 0) {
        // in ra man hinh server de kiem tra
        printf("\n[%s] Co ket noi tu IP: %s\n", time_str, client_ip);
        printf("Da nhan thong tin sinh vien:\n");
        printf("- MSSV: %s\n", sv.mssv);
        printf("- Ho ten: %s\n", sv.ho_ten);
        printf("- Ngay sinh: %s\n", sv.ngay_sinh);
        printf("- GPA: %.2f\n", sv.gpa);

        // mo tep tin luu du lieu (a: append mode)
        FILE *f_log = fopen(log_file, "a");
        if (f_log != NULL) {
            // Ghi toan bo tren 1 dong: IP Time MSSV HoTen NgaySinh GPA
            fprintf(f_log, "%s %s %s %s %s %.2f\n", client_ip, time_str,
                    sv.mssv, sv.ho_ten, sv.ngay_sinh, sv.gpa);
            fclose(f_log);
            printf("Da ghi log thanh cong\n");
        } else {
            perror("Khong the mo file log");
        }
    } else {
        perror("Loi khi nhan du lieu tu client");
    }

    // Dong ket noi
    close(client_sock);

    close(listen_sock);
    return 0;
}