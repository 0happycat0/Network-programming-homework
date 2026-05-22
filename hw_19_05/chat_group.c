#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int waiting_client = -1; // hang doi luu 1 client dang cho

typedef struct {
    int client1;
    int client2;
} ClientPair;

typedef struct {
    int sender;
    int receiver;
    ClientPair *pair;
} ForwardData;

void closePair(ClientPair *pair) {
    // dong ca 2 client
    if (pair->client1 != -1) {
        shutdown(pair->client1, SHUT_RDWR);
        close(pair->client1);
        pair->client1 = -1;
    }

    if (pair->client2 != -1) {
        shutdown(pair->client2, SHUT_RDWR);
        close(pair->client2);
        pair->client2 = -1;
    }
}

void *forwardMessage(void *arg) {
    ForwardData *data = (ForwardData *)arg;

    int sender = data->sender;
    int receiver = data->receiver;
    ClientPair *pair = data->pair;

    char buffer[1024];

    while (1) {
        // nhan tin nhan tu sender
        int bytes_received = recv(sender, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            printf("Client %d disconnected\n", sender);
            break;
        }

        buffer[bytes_received] = '\0';

        // gui tin nhan vua nhan sang receiver
        char send_message[1048];
        snprintf(send_message, sizeof(send_message), "Client %d: %s", sender, buffer);
        int bytes_sent = send(receiver, send_message, strlen(send_message), 0);

        if (bytes_sent <= 0) {
            printf("Client %d disconnected\n", receiver);
            break;
        }
    }

    // neu 1 client ngat ket noi thi dong ca 2 client
    closePair(pair);

    free(data);
    return NULL;
}

void *handlePair(void *arg) {
    ClientPair *pair = (ClientPair *)arg;

    printf("Pair created: client %d & client %d\n", pair->client1,
           pair->client2);

    char msg1[] = "You are paired. Start chatting...\n";
    char msg2[] = "You are paired. Start chatting...\n";

    send(pair->client1, msg1, strlen(msg1), 0);
    send(pair->client2, msg2, strlen(msg2), 0);

    pthread_t thread1, thread2;

    ForwardData *data1 = malloc(sizeof(ForwardData));
    data1->sender = pair->client1;
    data1->receiver = pair->client2;
    data1->pair = pair;

    ForwardData *data2 = malloc(sizeof(ForwardData));
    data2->sender = pair->client2;
    data2->receiver = pair->client1;
    data2->pair = pair;

    // tao 2 luong de chuyen tiep tin nhan 2 chieu
    pthread_create(&thread1, NULL, forwardMessage, data1);
    pthread_create(&thread2, NULL, forwardMessage, data2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Pair closed\n");

    free(pair);
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

        // neu chua co client nao dang cho thi dua client moi vao hang doi
        if (waiting_client == -1) {
            waiting_client = client;

            char msg[] = "Waiting for another client...\n";
            send(client, msg, strlen(msg), 0);

            printf("Client %d is waiting\n", client);
        } else {
            // neu da co 1 client dang cho thi ghep cap
            ClientPair *pair = malloc(sizeof(ClientPair));
            pair->client1 = waiting_client;
            pair->client2 = client;

            waiting_client = -1;

            pthread_t pair_thread;

            // moi cap client duoc xu ly boi 1 luong rieng
            pthread_create(&pair_thread, NULL, handlePair, pair);
            pthread_detach(pair_thread);
        }
    }

    close(listen_sock);
    return 0;
}