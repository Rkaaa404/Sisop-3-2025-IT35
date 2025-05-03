#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define SERVER_IP "127.0.0.1"
#define PORT 65432
#define BUFFER_SIZE 4096
#define MAX_FILENAME 256
#define MAX_FILEPATH 512
#define SECRETS_DIR "client/secrets"
#define DOWNLOADS_DIR "client"

// Function to ensure directory exists
void ensure_directory_exists(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == -1) {
        // Create parent directory first
        char parent_dir[MAX_FILEPATH];
        strcpy(parent_dir, dir);
        char *last_slash = strrchr(parent_dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            ensure_directory_exists(parent_dir);
        }
        
        // Create this directory
        mkdir(dir, 0755);
    }
}

// Function to connect to server
int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("ERROR: Socket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("ERROR: Invalid address or address not supported\n");
        close(sock);
        return -1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("ERROR: Connection failed. Make sure the server is running.\n");
        close(sock);
        return -1;
    }
    
    return sock;
}

// Function to send file for decryption
int send_file_for_decryption(int sock, const char *filepath) {
    char buffer[BUFFER_SIZE];
    FILE *file;
    struct stat st;
    char command[64];
    ssize_t bytes_read, bytes_sent;
    int success = 0;
    
    // Check if file exists
    if (stat(filepath, &st) == -1) {
        printf("ERROR: File %s not found\n", filepath);
        return 0;
    }
    
    // Open file
    file = fopen(filepath, "r");
    if (!file) {
        printf("ERROR: Unable to open file %s\n", filepath);
        return 0;
    }
    
    // Read file into buffer
    char *file_content = malloc(st.st_size + 1);
    if (!file_content) {
        printf("ERROR: Memory allocation failed\n");
        fclose(file);
        return 0;
    }
    
    bytes_read = fread(file_content, 1, st.st_size, file);
    fclose(file);
    
    if (bytes_read != st.st_size) {
        printf("ERROR: Failed to read entire file\n");
        free(file_content);
        return 0;
    }
    
    file_content[bytes_read] = '\0';
    
    // Send DECRYPT command with file size
    snprintf(command, sizeof(command), "DECRYPT:%ld", st.st_size);
    send(sock, command, strlen(command), 0);
    
    // Wait for server to be ready
    bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    
    if (strcmp(buffer, "READY") != 0) {
        printf("ERROR: Server not ready - %s\n", buffer);
        free(file_content);
        return 0;
    }
    
    // Send file content
    bytes_sent = send(sock, file_content, st.st_size, 0);
    free(file_content);
    
    if (bytes_sent != st.st_size) {
        printf("ERROR: Failed to send entire file\n");
        return 0;
    }
    
    // Wait for response
    bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "SUCCESS:", 8) == 0) {
        printf("SUCCESS: File decrypted and saved as %s\n", buffer + 9);
        success = 1;
    } else {
        printf("ERROR: %s\n", buffer);
    }
    
    return success;
}

// Function to download JPEG
int download_jpeg(int sock, const char *filename) {
    char buffer[BUFFER_SIZE];
    char command[MAX_FILENAME + 10];
    ssize_t bytes_read;
    int file_size = 0;
    int fd;
    int success = 0;
    char filepath[MAX_FILEPATH];
    
    // Send download command
    snprintf(command, sizeof(command), "DOWNLOAD:%s", filename);
    send(sock, command, strlen(command), 0);
    
    // Get response
    bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "ERROR:", 6) == 0) {
        printf("%s\n", buffer);
        return 0;
    }
    
    if (strncmp(buffer, "SIZE:", 5) == 0) {
        file_size = atoi(buffer + 5);
        
        // Create filepath
        snprintf(filepath, sizeof(filepath), "%s/%s", DOWNLOADS_DIR, filename);
        
        // Open file for writing
        fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            printf("ERROR: Unable to create file %s\n", filepath);
            return 0;
        }
        
        // Send ready signal
        send(sock, "READY", 5, 0);
        
        // Receive and write file data
        int total_received = 0;
        while (total_received < file_size) {
            bytes_read = recv(sock, buffer, BUFFER_SIZE < (file_size - total_received) ? 
                             BUFFER_SIZE : (file_size - total_received), 0);
            
            if (bytes_read <= 0) {
                printf("ERROR: Connection closed or error during download\n");
                close(fd);
                return 0;
            }
            
            if (write(fd, buffer, bytes_read) != bytes_read) {
                printf("ERROR: Failed to write to file\n");
                close(fd);
                return 0;
            }
            
            total_received += bytes_read;
        }
        
        close(fd);
        printf("SUCCESS: Downloaded %s (%d bytes)\n", filename, file_size);
        success = 1;
    } else {
        printf("ERROR: Unexpected response - %s\n", buffer);
    }
    
    return success;
}

// Function to display menu
void display_menu() {
    printf("\n===== Image Client Menu =====\n");
    printf("1. Decrypt text file\n");
    printf("2. Download JPEG file\n");
    printf("3. Exit\n");
    printf("Enter your choice (1-3): ");
}

// Function to list files in secrets directory
void list_secret_files() {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    
    dir = opendir(SECRETS_DIR);
    if (!dir) {
        printf("No files found in %s/\n", SECRETS_DIR);
        printf("Please place text files in the %s/ directory.\n", SECRETS_DIR);
        return;
    }
    
    printf("Available files in %s/:\n", SECRETS_DIR);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {  // Skip hidden files and . and ..
            printf("%d. %s\n", ++count, entry->d_name);
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        printf("No files found in %s/\n", SECRETS_DIR);
        printf("Please place text files in the %s/ directory.\n", SECRETS_DIR);
    }
}

int main() {
    int sock;
    int choice;
    char filename[MAX_FILENAME];
    char filepath[MAX_FILEPATH];
    
    // Ensure directories exist
    ensure_directory_exists(SECRETS_DIR);
    ensure_directory_exists(DOWNLOADS_DIR);
    
    printf("===== Image Client =====\n");
    printf("Server: %s:%d\n", SERVER_IP, PORT);
    
    // Connect to server
    sock = connect_to_server();
    if (sock < 0) {
        return 1;
    }
    
    // Main loop
    while (1) {
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        // Clear input buffer
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        switch (choice) {
            case 1:
                // Decrypt text file
                printf("\n--- Decrypt Text File ---\n");
                list_secret_files();
                
                printf("Enter filename to decrypt: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    // Remove newline
                    filename[strcspn(filename, "\n")] = 0;
                    
                    if (strlen(filename) > 0) {
                        snprintf(filepath, sizeof(filepath), "%s/%s", SECRETS_DIR, filename);
                        send_file_for_decryption(sock, filepath);
                    }
                }
                break;
                
            case 2:
                // Download JPEG file
                printf("\n--- Download JPEG File ---\n");
                printf("Enter JPEG filename to download (e.g., 1744401234.jpeg): ");
                
                if (fgets(filename, sizeof(filename), stdin)) {
                    // Remove newline
                    filename[strcspn(filename, "\n")] = 0;
                    
                    if (strlen(filename) > 0) {
                        // Add .jpeg extension if missing
                        if (strstr(filename, ".jpeg") == NULL) {
                            strcat(filename, ".jpeg");
                        }
                        
                        download_jpeg(sock, filename);
                    }
                }
                break;
                
            case 3:
                // Exit
                printf("Exiting client...\n");
                send(sock, "EXIT", 4, 0);
                close(sock);
                return 0;
                
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }
    
    close(sock);
    return 0;
}