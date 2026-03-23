#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char mssv[16];
    char ho_ten[64];
    char ngay_sinh[16];
    float gpa;
} SinhVien;

// sv client
int main(int argc, char *argv[]) {
    char *cmd = argv[1];
    char *server_ip = argv[2];
    int server_port = atoi(argv[3]);

    // kiem tra lenh
    if (strcmp(cmd, "sv_client") != 0 || argc != 4) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    // tao socket
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

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
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        exit(EXIT_FAILURE);
    }

    // nhap du lieu thong tin sinh vien tu ban phim
    SinhVien sv;
    memset(&sv, 0, sizeof(SinhVien));

    printf("Nhap thong tin sinh vien:\n");
    
    printf("MSSV: ");
    fgets(sv.mssv, sizeof(sv.mssv), stdin);
    sv.mssv[strcspn(sv.mssv, "\n")] = 0; // Xoa ky tu enter (newline) thua o cuoi chuoi

    printf("Ho ten: ");
    fgets(sv.ho_ten, sizeof(sv.ho_ten), stdin);
    sv.ho_ten[strcspn(sv.ho_ten, "\n")] = 0;

    printf("Ngay sinh: ");
    fgets(sv.ngay_sinh, sizeof(sv.ngay_sinh), stdin);
    sv.ngay_sinh[strcspn(sv.ngay_sinh, "\n")] = 0;

    printf("Diem trung binh: ");
    scanf("%f", &sv.gpa);

    // gui du lieu cho server
    if (send(sock, &sv, sizeof(SinhVien), 0) < 0) {
        perror("send() failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("\nDa thong tin den server\n");

    // dong ket noi
    close(sock);
    return 0;
}