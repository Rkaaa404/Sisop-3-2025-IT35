#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define IP "127.0.0.1"

int get_int_input(int sock) {
    char input_buffer[100];
    int choice = -1;

    while (1) {
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            perror("Error reading input");
            return -1;
        }

        if (sscanf(input_buffer, "%d", &choice) == 1) {
            send(sock, &choice, sizeof(int), 0);
            return choice;
        } else {
            fprintf(stderr, "Invalid input. Please enter a number.\n");
        }
    }
}

void get_string_input(int sock, char *buffer, size_t buffer_size) {
    if (fgets(buffer, buffer_size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        send(sock, buffer, strlen(buffer) + 1, 0);
    } else {
        perror("Error reading input string");
        buffer[0] = '\0';
        send(sock, buffer, 1, 0);
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char buffer[1024] = {0};
        char input_buffer[100];
        int choice;

        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Choose an option:") != NULL) {
             choice = get_int_input(sock);

             if (choice == -1) break;

            if (choice == 5) {
                printf("Disconnecting...\n");
                break;
            }
        } else if (strstr(buffer, "Choose weapon number to buy:") != NULL) {
             int weapon_choice = get_int_input(sock);
             if (weapon_choice == -1) break;
        } else if (strstr(buffer, "Choose weapon number to use:") != NULL) {
             int inv_choice = get_int_input(sock);
             if (inv_choice == -1) break;
        } else if (strstr(buffer, "Options: attack | exit") != NULL) {
            get_string_input(sock, input_buffer, sizeof(input_buffer));
            if (strcmp(input_buffer, "exit") == 0) {
                // Server will handle battle exit
            }
        }
    }

    close(sock);
    return 0;
}
