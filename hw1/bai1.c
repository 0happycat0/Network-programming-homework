#include <stdio.h>

int main() {
    char inp[50];
    char cmd[16];
    char ip_inp[16];
    int port;

    printf("Nhap lenh: ");
    fgets(inp, sizeof(inp), stdin);
    int n = sscanf(inp, "%s %s %d", cmd, ip_inp, &port); // so bien phan tich duoc

    printf("n=%d\n", n);
    printf("cmd=%s\n", cmd);
    printf("ip_inp=%s\n", ip_inp);
    printf("port=%d\n", port);

    // kiem tra lenh
    if(cmd != "tcp_client" || n != 3) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    return 0;
}