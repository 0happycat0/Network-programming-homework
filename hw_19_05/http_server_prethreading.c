#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void *thread_proc(void *arg) {
    int listen_sock = *(int *)arg;
    char buffer[1024];

    while (1) {
        // cho ket noi
        // tat ca cac luong cung cho o cung 1 socket
        int client = accept(listen_sock, NULL, NULL);

        if (client < 0)
            continue;

        printf("New client accepted in thread %ld with pid %d: %d\n",
               pthread_self(), getpid(), client);

        // cho du lieu tu client
        int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            close(client);
            continue;
        }

        buffer[bytes_received] = '\0';

        puts(buffer);

        // tra lai ket qua cho client
        char *msg = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "\r\n"
                    "<html><body><h1>Xin chao cac ban</h1></body></html>";

        send(client, msg, strlen(msg), 0);

        close(client);
    }

    return NULL;
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

    int num_threads = 8;
    pthread_t thread_id[8];

    // tao san nhieu luong de xu ly client
    for (int i = 0; i < num_threads; i++) {
        int ret =
            pthread_create(&thread_id[i], NULL, thread_proc, &listen_sock);
        if (ret != 0) {
            printf("Could not create new thread.\n");
            continue;
        }
    }

    printf("Press Enter to exit...\n");
    getchar();

    close(listen_sock);

    return 0;
}