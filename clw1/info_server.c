#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// info server
int main() {
    int server_port = 8888;

    // tao socket
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // khai bao dia chi server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // lang nghe tren moi IP
    server_addr.sin_port = htons(server_port);

    // gan dia chi voi socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // lang nghe ket noi
    if (listen(server_sock, 5) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening at port %d...\n", server_port);

    // chap nhan ket noi tu client
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_sock < 0) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
    }

    printf("Client is connected\n");

    // nhan du lieu tu client
    char buffer[65536];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
    
    if (bytes_received < 0) {
        perror("recv() failed");
    } else if (bytes_received == 0) {
        printf("Client is disconnected.\n");
    } else {
        int offset = 0;

        // lay ten thu muc (ket thuc bang \0)
        char *dir_name = buffer;
        printf("\nDa nhan duoc du lieu...\n");
        printf("Thu muc hien tai: %s\n", dir_name);
        
        // dich chuyen offset qua khoi ten thu muc va ky tu \0
        offset += strlen(dir_name) + 1;

        printf("Danh sach cac tap tin:\n");

        // doc thong tin file
        while (offset < bytes_received) {
            // lay ten file
            char *file_name = buffer + offset;
            
            // neu gap chuoi rong thi ket thuc
            if (strlen(file_name) == 0) {
                break; 
            }
            
            offset += strlen(file_name) + 1;

            // lay kich thuoc file
            int net_size;
            memcpy(&net_size, buffer + offset, sizeof(int));
            
            // dich nguoc tu Network Byte Order ve Host Byte Order
            int file_size = ntohl(net_size); 
            
            offset += sizeof(int);

            // in ket qua ra man hinh
            printf(" - %s : %d bytes\n", file_name, file_size);
        }
    }

    // dong socket
    close(client_sock);
    close(server_sock);

    return 0;
}