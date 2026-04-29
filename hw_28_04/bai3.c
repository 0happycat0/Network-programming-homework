#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_TOPICS 10

void removeClient(struct pollfd *fds, char clients_topic[][MAX_TOPICS][64],
                  int *num_fds, int i) {
    // ham xoa phan tu thu i cua mang (khong giu thu tu mang)
    // bang cach: ghi de phan tu thu i bang phan tu cuoi roi giam size di 1
    if (i < *num_fds - 1) {
        fds[i] = fds[*num_fds - 1];
        // copy toan bo danh sach topic
        for (int k = 0; k < MAX_TOPICS; k++) {
            strcpy(clients_topic[i][k], clients_topic[*num_fds - 1][k]);
        }
    }
    *num_fds -= 1;
}

int main() {
    int port = 9000;

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    // Thiết lập SO_REUSEADDR để giải phóng cổng ngay khi server tắt
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt() failed");
        close(listen_sock);
        return 1;
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

    // Su dung mang 3 chieu: id client - id topic - noi dung topic
    char clients_topic[64][MAX_TOPICS][64];
    int num_fds = 1; // so fd

    fds[0].fd = listen_sock; // dua listen_sock vao danh sach theo doi
    fds[0].events = POLLIN;

    char buffer[1024];

    while (1) {
        if (poll(fds, num_fds, -1) < 0) {
            perror("poll() failed");
            break;
        }

        // neu co yeu cau ket noi
        if (fds[0].revents & POLLIN) {
            int client = accept(listen_sock, NULL, NULL);
            if (num_fds < max_client) {
                printf("New client connected: %d\n", client);
                fds[num_fds].fd = client;
                fds[num_fds].events = POLLIN;

                // khoi tao topic cua client nay la chuoi rong
                for (int k = 0; k < MAX_TOPICS; k++) {
                    strcpy(clients_topic[num_fds][k], "");
                }

                num_fds++;

                char welcome[] = "Connected, send SUB <topic>, "
                                 "UNSUB <topic> or PUB <topic> <msg>\n";
                send(client, welcome, strlen(welcome), 0);
            } else {
                close(client);
            }
        }

        // kiem tra nhan du lieu tu client
        for (int i = 1; i < num_fds; i++) {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                int bytes_received =
                    recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_received <= 0) {
                    printf("Client number %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);
                    removeClient(fds, clients_topic, &num_fds, i);
                    i--;
                    continue;
                }

                buffer[bytes_received] = '\0';
                // bo ky tu '\n' va '\r'
                buffer[strcspn(buffer, "\r\n")] = 0;

                // bo qua neu client chi nhan Enter
                if (strlen(buffer) == 0)
                    continue;

                // client gui lenh SUB <topic>
                if (strncmp(buffer, "SUB ", 4) == 0) {
                    // tach cac topic bang dau cach " "
                    char *topic_ptr = strtok(buffer + 4, " ");

                    while (topic_ptr != NULL) {
                        if (strlen(topic_ptr) > 0) {
                            int empty_idx = -1;
                            int is_exist = 0;

                            // duyet cac o luu topic de tim cho trong
                            for (int k = 0; k < MAX_TOPICS; k++) {
                                if (strcmp(clients_topic[i][k], topic_ptr) ==
                                    0) {
                                    is_exist = 1;
                                    break;
                                }
                                if (empty_idx == -1 &&
                                    strlen(clients_topic[i][k]) == 0) {
                                    empty_idx = k; // luu lai o con trong
                                }
                            }

                            if (is_exist) {
                                char sub_ack[128];
                                snprintf(sub_ack, sizeof(sub_ack),
                                         "You have already subscribed to topic "
                                         "'%s'\n",
                                         topic_ptr);
                                send(fds[i].fd, sub_ack, strlen(sub_ack), 0);
                            } else if (empty_idx != -1) {
                                // ghi topic moi vao o trong vua tim duoc
                                strcpy(clients_topic[i][empty_idx], topic_ptr);
                                printf("Client %d subscribed to topic: '%s'\n",
                                       fds[i].fd, topic_ptr);

                                char sub_ack[128];
                                snprintf(sub_ack, sizeof(sub_ack),
                                         "Topic subscribed: %s\n", topic_ptr);
                                send(fds[i].fd, sub_ack, strlen(sub_ack), 0);
                            } else {
                                // het cho luu topic
                                char sub_ack[128];
                                snprintf(sub_ack, sizeof(sub_ack),
                                         "Cannot subscribe to '%s': maximum "
                                         "topics reached\n",
                                         topic_ptr);
                                send(fds[i].fd, sub_ack, strlen(sub_ack), 0);
                            }
                        }
                        // tach topic tiep theo
                        topic_ptr = strtok(NULL, " ");
                    }
                }

                // client gui lenh UNSUB <topic>
                else if (strncmp(buffer, "UNSUB ", 6) == 0) {
                    char *topic_ptr = strtok(buffer + 6, " ");
                    
                    while (topic_ptr != NULL) {
                        if (strlen(topic_ptr) > 0) {
                            int found = 0;

                            // tim topic do trong danh sach va xoa no ve chuoi rong
                            for (int k = 0; k < MAX_TOPICS; k++) {
                                if (strcmp(clients_topic[i][k], topic_ptr) == 0) {
                                    strcpy(clients_topic[i][k], "");
                                    found = 1;
                                    break;
                                }
                            }

                            if (found) {
                                printf("Client %d unsubscribed from topic: '%s'\n", fds[i].fd, topic_ptr);
                                char unsub_ack[128];
                                snprintf(unsub_ack, sizeof(unsub_ack),
                                         "Topic unsubscribed: %s\n", topic_ptr);
                                send(fds[i].fd, unsub_ack, strlen(unsub_ack), 0);
                            } else {
                                char unsub_ack[128];
                                snprintf(unsub_ack, sizeof(unsub_ack),
                                         "You have not subscribed to topic '%s'\n", topic_ptr);
                                send(fds[i].fd, unsub_ack, strlen(unsub_ack), 0);
                            }
                        }
                        // tach topic tiep theo
                        topic_ptr = strtok(NULL, " ");
                    }
                }

                // client gui lenh PUB <topic> <msg>
                else if (strncmp(buffer, "PUB ", 4) == 0) {
                    char *topic_ptr = buffer + 4;
                    char *space_ptr = strchr(topic_ptr, ' ');

                    if (space_ptr != NULL) {
                        *space_ptr = '\0'; // tach topic
                        char *msg_ptr = space_ptr + 1;

                        printf("Client %d published to '%s': %s\n", fds[i].fd,
                               topic_ptr, msg_ptr);

                        char send_buf[1050];
                        snprintf(send_buf, sizeof(send_buf), "[%s] %s\n",
                                 topic_ptr, msg_ptr);

                        // duyet tat ca cac client
                        for (int j = 1; j < num_fds; j++) {
                            // voi moi client duyet tat ca cac topic no dang
                            // ky
                            for (int k = 0; k < MAX_TOPICS; k++) {
                                // neu topic k cua client j trung voi topic duoc
                                // pub
                                if (strcmp(clients_topic[j][k], topic_ptr) ==
                                    0) {
                                    send(fds[j].fd, send_buf, strlen(send_buf),
                                         0);
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    char err_msg[] = "Syntax error. Use: SUB <topic>, UNSUB "
                                     "<topic> or PUB <topic> <msg>\n";
                    send(fds[i].fd, err_msg, strlen(err_msg), 0);
                }
            }
        }
    }

    close(listen_sock);
    return 0;
}