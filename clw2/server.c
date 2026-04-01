#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    // Chuyen socket listener sang non-blocking
    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                   sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    // Server is now listening for incoming connections
    printf("Server is listening on port 8080...\n");

    int clients[64];
    int nclients = 0;

    char buf[256];
    int len;

    while (1) {
        // Chap nhan ket noi
        int client = accept(listener, NULL, NULL);
        if (client == -1) {
            if (errno == EWOULDBLOCK) {
                // Loi do dang cho ket noi
                // Bo qua
            } else {
                // Loi khac
            }
        } else {
            printf("New client accepted: %d\n", client);
            char *ask_msg =
                "Hay nhap Ho ten - MSSV (Vi du: Luu Ngoc Loi-20225357): ";
            send(client, ask_msg, strlen(ask_msg), 0);
            clients[nclients] = client;
            nclients++;
            ul = 1;
            ioctl(client, FIONBIO, &ul);
        }

        // Nhan du lieu tu cac client
        for (int i = 0; i < nclients; i++) {
            len = recv(clients[i], buf, sizeof(buf), 0);
            if (len == -1) {
                if (errno == EWOULDBLOCK) {
                    // Loi do cho du lieu
                    // Bo qua
                } else {
                    continue;
                }
            } else {
                if (len == 0)
                    continue;
                buf[len] = 0;

                char fullname[64] = {0};
                char mssv[10] = {0};
                char email[100] = {0};

                char *token = strtok(buf, "-");
                if (token != NULL) {
                    // lay ho va ten
                    strcpy(fullname, token);
                    token = strtok(NULL, "-");
                    if (token != NULL)
                        // lay mssv
                        strcpy(mssv, token);
                }

                char first_letters[10] = "";
                char last_name[10] = "";

                // xu ly ho ten (tach ten)
                char *name_token = strtok(fullname, " ");
                while (name_token != NULL) {
                    char *next = strtok(NULL, " ");
                    if (next != NULL) {
                        // lay chu cai dau cua ho va dem
                        char initial = (char)tolower(name_token[0]);
                        strncat(first_letters, &initial, 1);
                        name_token = next;
                    } else {
                        // ten chinh
                        strcpy(last_name, name_token);
                        for (int j = 0; last_name[j]; j++)
                            last_name[j] = tolower(last_name[j]);
                        name_token = NULL;
                    }
                }

                // xu ly mssv
                char mssv_short[10];
                sprintf(mssv_short, "%s", mssv + 2);

                char *check_enter = &mssv_short[strlen(mssv_short) - 1];
                if (*check_enter == '\n') { // bo ky tu enter do client gui di
                    *check_enter = '\0';
                }

                // tra ve email cho client
                sprintf(email, "%s.%s%s@sis.hust.edu.vn\n", last_name,
                        first_letters, mssv_short);
                send(clients[i], email, strlen(email), 0);
            }
        }
    }

    close(listener);
    return 0;
}