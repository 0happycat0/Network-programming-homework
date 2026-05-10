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

    int num_processes = 8;
    char buffer[1024];

    signal(SIGCHLD, signalHandler);

    for (int i = 0; i < num_processes; i++) {
        if (fork() == 0) {
            while (1) {
                // cho ket noi
                // tat ca cac tien trinh con cung cho o cung 1 socket
                int client = accept(listen_sock, NULL, NULL);
                if (client < 0) continue;

                printf("New client accepted in process %d: %d\n", getpid(), client);

                int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received <= 0) {
                    close(client);
                    continue;
                }

                buffer[bytes_received] = '\0';
                
                puts(buffer); 

                // tra lai ket qua cho client
                char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xinchao cac ban</h1></body></html>";
                send(client, msg, strlen(msg), 0);

                close(client);
            }
            exit(0); 
        }
    }

    printf("Press Enter to exit...\n");
    getchar();

    killpg(0, SIGKILL); // kill het tat ca tien trinh

    close(listen_sock);
    return 0;
}