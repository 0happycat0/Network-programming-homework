#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int removeClient(struct pollfd *fds, int *num_fds, int i) {
    if (i < *num_fds - 1) {
        fds[i] = fds[*num_fds - 1];
    }
    *num_fds -= 1;
}

char *encodeMsg(char msg[], int size) {
    for(int i = 0; i < size; i++) {
        if (msg[i] >= 'a' && msg[i] <= 'y') {
            msg[i]++;
        } else if (msg[i] == 'z') {
            msg[i] = 'a';
        } else if (msg[i] >= 'A' && msg[i] <= 'Y') {
            msg[i]++;
        } else if (msg[i] == 'Z') {
            msg[i] = 'A';
        } else if (msg[i] >= '0' && msg[i] <= '9') {
            msg[i] = '9' - (msg[i] - '0'); 
        }
    }
    return msg;
}

int main() {
    int port = 8888;

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

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

    // lang nghe voi hang doi do dai 5
    if (listen(listen_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);

    const int max_client = 63;
    struct pollfd fds[64]; // luu fd cua client (mang cac client can theo doi)
    int num_fds = 1;       // so fd

    fds[0].fd = listen_sock; // dua listen_sock vao danh sach theo doi
    fds[0].events = POLLIN;

    char buffer[1024];
    char hello_message[] = "Hello client...\n\0";

    while (1) {
        // theo doi cac fd voi thoi gian cho vo han
        if (poll(fds, num_fds, -1) < 0) {
            perror("poll() failed");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client = accept(listen_sock, NULL, NULL);
            if (num_fds - 1 < max_client) {
                printf("New client connected: %d\n", client);
                fds[num_fds].fd = client;
                fds[num_fds].events = POLLIN;
                num_fds++;

                char greeting_message[128];
                snprintf(greeting_message, sizeof(greeting_message), "Hello client. %d clients are connecting\n", num_fds - 1);
                send(client, greeting_message, strlen(greeting_message), 0);
            } else {
                close(client);
            }
        }

        // kiem tra nhan du lieu tu client
        for (int i = 1; i < num_fds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                int bytes_received = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes_received < 0) {
                    printf("Client number %d disconnected\n", fds[i].fd);
                    removeClient(fds, &num_fds, i);
                    i--;
                    continue;
                }

                buffer[bytes_received] = '\0';
                // bo ky tu '\n'
                buffer[strcspn(buffer, "\r\n")] = 0;
                printf("Received from client %d: %s\n", fds[i].fd, buffer);

                // client nhap exit thi dong ket noi
                if (strcmp(buffer, "exit") == 0) {
                    char goodbye_message[] = "Goodbye\n";
                    send(fds[i].fd, goodbye_message, strlen(goodbye_message), 0);

                    printf("Client number %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);
                    removeClient(fds, &num_fds, i);
                    i--;
                    continue;
                }

                encodeMsg(buffer, sizeof(buffer));
                char send_buf[1050];
                snprintf(send_buf, sizeof(send_buf), "%s\n", buffer);
                send(fds[i].fd, send_buf, strlen(send_buf), 0);
            }
        }
    }

    close(listen_sock);
    return 0;
}