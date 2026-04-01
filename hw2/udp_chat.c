#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *cmd = argv[1];
    int my_port = atoi(argv[2]);
    char *peer_ip = argv[3];
    int peer_port = atoi(argv[4]);

    // kiem tra lenh
    if (strcmp(cmd, "udp_chat") != 0 || argc != 5) {
        printf("Nhap sai lenh\n");
        return 0;
    }

    // tao socket UDP
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    unsigned long ul = 1;

    // Chuyen socket sang non-blocking
    ioctl(sock, FIONBIO, &ul);

    // chuyen ban phim sang non blocking
    ioctl(STDIN_FILENO, FIONBIO, &ul);

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(sock);
        return 1;
    }

    // khai bao va gan dia chi ung dung cua minh
    struct sockaddr_in my_addr = {0};
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(my_port);

    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao chi cua friend
    struct sockaddr_in peer_addr = {0};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

    printf("\nDang hoat dong tren cong %d, gui toi cong %d\n", my_port,
           peer_port);
    printf("Nhap tin nhan roi nhan enter de gui (nhap 'exit' de thoat):\n\n");

    char buf_keyboard[1024];
    char buf_receive[1024];

    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (1) {
        // read non blocking
        int bytes_read =
            read(STDIN_FILENO, buf_keyboard, sizeof(buf_keyboard) - 1);
        if (bytes_read > 0) {
            buf_keyboard[bytes_read] = '\0';

            // go exit
            if (strncmp(buf_keyboard, "exit", 4) == 0) {
                printf("Exit.\n");
                break;
            }

            // gui data
            sendto(sock, buf_keyboard, sizeof(buf_keyboard), 0,
                   (struct sockaddr *)&peer_addr, sizeof(peer_addr));
        }

        int bytes_received =
            recvfrom(sock, buf_receive, sizeof(buf_receive), 0,
                     (struct sockaddr *)&sender_addr, &sender_len);

        if (bytes_received <= 0) {
            if (errno == EWOULDBLOCK) {
                // loi do doi data thi bo qua
            } else {
                continue; // loi khac
            }
        } else {
            buf_receive[bytes_received] = '\0';
            int sender_port = ntohs(sender_addr.sin_port);
            // in ra man hinh tin nhan nhan duoc
            printf("Tu cong %d: %s\n", sender_port, buf_receive);
        }

        usleep(10000); // ngu 10 mili sec de giam tai CPU
    }
    close(sock);
    return 0;
}