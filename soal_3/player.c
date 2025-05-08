#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define IP "127.0.0.1"

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char input[10];

    // Membuat socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    // Terhubung ke server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_len = recv(sock, buffer, sizeof(buffer), 0);
        if (recv_len <= 0) break;  // koneksi ditutup oleh server

        printf("%s", buffer);  // tampilkan pesan dari server
        fflush(stdout);
        
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        send(sock, input, strlen(input), 0);
    }

    close(sock);
    return 0;
}
