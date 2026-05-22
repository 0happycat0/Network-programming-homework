#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

int clients[MAX_CLIENTS];              // luu fd cua client
int num_clients = 0;                   // so client
char clients_name[MAX_CLIENTS][128];   // ten client
bool clients_check[MAX_CLIENTS] = {0}; // check xem client nhap dung chua

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

char ask_name[] = "Please enter client id (format: client_id:client_name)\n";

int removeClient(int client) {
    // ham xoa client khoi mang (khong giu thu tu mang)
    // bang cach: ghi de client can xoa bang client cuoi roi giam size di 1
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            if (i < num_clients - 1) {
                clients[i] = clients[num_clients - 1];
                clients_check[i] = clients_check[num_clients - 1];
                strcpy(clients_name[i], clients_name[num_clients - 1]);
            }

            num_clients--;
            return 1;
        }
    }

    return 0;
}

int getClientIndex(int client) {
    // tim vi tri client trong mang clients
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] == client) {
            return i;
        }
    }

    return -1;
}

void sendToOtherClients(int sender, char *sender_name, char *message) {
    // thoi gian hien tai
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[64];

    // Dinh dang thoi gian: 2023/05/06 11:00:00PM
    strftime(time_str, sizeof(time_str), "%Y/%m/%d %I:%M:%S%p", t);

    // tao buffer gui
    char send_buf[2048];
    snprintf(send_buf, sizeof(send_buf), "%s %s: %s\n", time_str, sender_name,
             message);

    pthread_mutex_lock(&clients_mutex);

    // gui den cac client khac da dang nhap thanh cong
    for (int i = 0; i < num_clients; i++) {
        if (clients[i] != sender && clients_check[i]) {
            send(clients[i], send_buf, strlen(send_buf), 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handleClient(void *arg) {
    int client = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];

    // hoi ten client
    send(client, ask_name, strlen(ask_name), 0);

    while (1) {
        // kiem tra nhan du lieu tu client
        int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            printf("Client number %d disconnected\n", client);

            pthread_mutex_lock(&clients_mutex);
            removeClient(client);
            pthread_mutex_unlock(&clients_mutex);

            close(client);
            break;
        }

        buffer[bytes_received] = '\0';

        // bo ky tu '\n' va '\r'
        buffer[strcspn(buffer, "\r\n")] = 0;

        // neu nguoi dung chi go enter thi bo qua
        if (strlen(buffer) == 0)
            continue;

        printf("Received from client %d: %s\n", client, buffer);

        pthread_mutex_lock(&clients_mutex);

        int index = getClientIndex(client);

        if (index == -1) {
            pthread_mutex_unlock(&clients_mutex);
            close(client);
            break;
        }

        // kiem tra neu client chua dang nhap thanh cong
        if (!clients_check[index]) {
            // kiem tra cu phap: "client_id: client_name"
            if (strncmp(buffer, "client_id:", 10) == 0) {
                char *name_ptr = buffer + 10;

                if (*name_ptr == ' ') // bo qua ' '
                    name_ptr++;

                if (strlen(name_ptr) > 0) {
                    clients_check[index] = true;
                    strcpy(clients_name[index], name_ptr);

                    printf("Client %d registered as '%s'\n", client,
                           clients_name[index]);

                    char success_msg[] = "Bat dau chat:\n";
                    send(client, success_msg, strlen(success_msg), 0);

                    pthread_mutex_unlock(&clients_mutex);
                    continue;
                }
            }

            pthread_mutex_unlock(&clients_mutex);

            // neu sai cu phap, tiep tuc nhap lai
            send(client, ask_name, strlen(ask_name), 0);
        } else {
            char sender_name[128];

            // copy ten client ra bien rieng de tranh giu mutex qua lau
            strcpy(sender_name, clients_name[index]);

            pthread_mutex_unlock(&clients_mutex);

            // gui du lieu den cac client khac
            sendToOtherClients(client, sender_name, buffer);
        }
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

    while (1) {
        // neu co yeu cau ket noi
        int client = accept(listen_sock, NULL, NULL);

        if (client < 0) {
            perror("accept() failed");
            continue;
        }

        printf("New client accepted: %d\n", client);

        pthread_mutex_lock(&clients_mutex);

        if (num_clients >= MAX_CLIENTS) {
            pthread_mutex_unlock(&clients_mutex);

            char full_msg[] = "Server is full\n";
            send(client, full_msg, strlen(full_msg), 0);
            close(client);
            continue;
        }

        // chap nhan va ghi vao mang clients
        clients[num_clients] = client;
        clients_check[num_clients] = false;
        strcpy(clients_name[num_clients], "");
        num_clients++;

        pthread_mutex_unlock(&clients_mutex);

        pthread_t thread_id;

        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client;

        // tao luong rieng de xu ly client
        pthread_create(&thread_id, NULL, handleClient, client_ptr);
        pthread_detach(thread_id);
    }

    close(listen_sock);
    return 0;
}