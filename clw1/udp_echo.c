#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

// udp_echo
int main() {
    int my_port, friend_port;
    char *peer_ip = "127.0.0.1";

    // nhap cong cua chinh minh
    printf("nhap cong ung dung: ");
    scanf("%d", &my_port);

    // nhap cong cua friend
    printf("nhap cong dich de gui: ");
    scanf("%d", &friend_port);

    while (getchar() != '\n')
        ;

    // tao socket UDP
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao va gan dia chi cua minh
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    my_addr.sin_port = htons(my_port);

    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao chi cua friend
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(friend_port);
    inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

    printf("\nDang hoat dong tren cong %d, gui toi cong %d\n", my_port,
           friend_port);
    printf("Nhap tin nhan roi nhan Enter de gui (nhap 'exit' de thoat):\n\n");

    // khai bao file descriptor
    fd_set read_fds;
    int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;
    char buffer[1024];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds); // kenh 1: lang nghe ban phim
        FD_SET(sock, &read_fds);         // kenh 2: lang nghe mang UDP

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select() failed");
            break;
        }

        // neu la tu ban phim
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // xoa ky tu xuong dong

            if (strcmp(buffer, "exit") == 0) {
                break;
            }

            // gui tin nhan den cong dich
            sendto(sock, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        }

        // neu la tu UDP
        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, sizeof(buffer));
            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);

            int bytes_received =
                recvfrom(sock, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&sender_addr, &sender_len);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                int sender_port = ntohs(sender_addr.sin_port);

                // in ra man hinh tin nhan nhan duoc
                printf("Tu cong %d: %s\n", sender_port, buffer);

                // them tien to echo de tranh vong lap vo han
                // neu trong buffer khong co (echo)
                if (strncmp(buffer, "(echo)", 6) != 0) {
                    char echo_msg[1024];
                    sprintf(echo_msg, "(echo) %s", buffer);

                    sendto(sock, echo_msg, strlen(echo_msg), 0,
                           (struct sockaddr *)&sender_addr, sender_len);
                }
            }
        }
    }

    close(sock);
    return 0;
}