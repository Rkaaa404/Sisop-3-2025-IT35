#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>

#define PORT 65432
#define BUFFER_SIZE 4096
#define MAX_FILENAME 256
#define MAX_FILEPATH 512
#define DATABASE_DIR "server/database"
#define LOG_FILE "server/server.log"

// Function to write to log file
void log_message(const char *source, const char *action, const char *message) {
    FILE *log_fp;
    time_t now;
    struct tm *time_info;
    char timestamp[20];
    
    time(&now);
    time_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
    
    log_fp = fopen(LOG_FILE, "a");
    if (log_fp) {
        fprintf(log_fp, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, message);
        fclose(log_fp);
    }
}

// Function to reverse a string
void reverse_string(char *str, int length) {
    int i, j;
    char temp;
    
    for (i = 0, j = length - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

// Function to convert hex string to binary
int hex_to_bin(const char *hex, unsigned char *bin, int hex_len) {
    int i;
    int bin_len = 0;
    
    for (i = 0; i < hex_len - 1; i += 2) {
        unsigned char byte;
        if (sscanf(&hex[i], "%2hhx", &byte) == 1) {
            bin[bin_len++] = byte;
        } else {
            return -1;  // Error in hex conversion
        }
    }
    
    return bin_len;
}

// Function to handle DECRYPT command
void handle_decrypt(int client_socket, char *data, int data_len) {
    char log_buffer[256];
    time_t timestamp;
    char filename[MAX_FILENAME];
    char filepath[MAX_FILEPATH];
    int result;
    int fd;
    
    // Log received data
    snprintf(log_buffer, sizeof(log_buffer), "Text data received (%d bytes)", data_len);
    log_message("Client", "DECRYPT", log_buffer);
    
    // Reverse the string
    reverse_string(data, data_len);
    
    // Allocate buffer for binary data (maximum size would be half of the hex string)
    unsigned char *binary_data = malloc(data_len / 2 + 1);
    if (!binary_data) {
        const char *error_msg = "Memory allocation failed";
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Convert hex to binary
    int binary_len = hex_to_bin(data, binary_data, data_len);
    if (binary_len < 0) {
        const char *error_msg = "Invalid hex data after reversal";
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        free(binary_data);
        return;
    }
    
    // Generate filename using timestamp
    timestamp = time(NULL);
    snprintf(filename, sizeof(filename), "%ld.jpeg", timestamp);
    snprintf(filepath, sizeof(filepath), "%s/%s", DATABASE_DIR, filename);
    
    // Save as JPEG
    fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to create file: %s", strerror(errno));
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        free(binary_data);
        return;
    }
    
    result = write(fd, binary_data, binary_len);
    close(fd);
    free(binary_data);
    
    if (result < 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to write file: %s", strerror(errno));
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Log success
    log_message("Server", "SAVE", filename);
    
    // Send success response
    char response[MAX_FILENAME + 10];
    snprintf(response, sizeof(response), "SUCCESS: %s", filename);
    send(client_socket, response, strlen(response), 0);
}

// Function to handle DOWNLOAD command
void handle_download(int client_socket, const char *filename) {
    char filepath[MAX_FILEPATH];
    char buffer[BUFFER_SIZE];
    struct stat st;
    int fd;
    ssize_t bytes_read;
    char size_msg[64];
    
    // Log download request
    log_message("Client", "DOWNLOAD", filename);
    
    // Build the filepath
    snprintf(filepath, sizeof(filepath), "%s/%s", DATABASE_DIR, filename);
    
    // Check if file exists
    if (stat(filepath, &st) == -1) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERROR: File %s not found", filename);
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Open the file
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "ERROR: Cannot open file %s", filename);
        log_message("Server", "ERROR", error_msg);
        send(client_socket, error_msg, strlen(error_msg), 0);
        return;
    }
    
    // Send file size
    snprintf(size_msg, sizeof(size_msg), "SIZE:%ld", st.st_size);
    send(client_socket, size_msg, strlen(size_msg), 0);
    
    // Wait for ready confirmation
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0 || strncmp(buffer, "READY", 5) != 0) {
        close(fd);
        log_message("Server", "ERROR", "Client not ready for file transfer");
        return;
    }
    
    // Send the file
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        if (send(client_socket, buffer, bytes_read, 0) != bytes_read) {
            log_message("Server", "ERROR", "Failed to send file data");
            break;
        }
    }
    
    close(fd);
    log_message("Server", "UPLOAD", filename);
}

// Function to handle client connection
void *handle_client(void *socket_desc) {
    int client_socket = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    char client_address[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    free(socket_desc);  // Free memory allocated in main
    
    // Get client IP
    getpeername(client_socket, (struct sockaddr*)&addr, &addr_len);
    inet_ntop(AF_INET, &(addr.sin_addr), client_address, INET_ADDRSTRLEN);
    
    // Log connection
    char log_buffer[256];
    snprintf(log_buffer, sizeof(log_buffer), "New connection from %s:%d", client_address, ntohs(addr.sin_port));
    log_message("Server", "CONNECT", log_buffer);
    
    // Handle client commands
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        
        // Handle DECRYPT command
        if (strncmp(buffer, "DECRYPT:", 8) == 0) {
            int file_size = atoi(buffer + 8);
            send(client_socket, "READY", 5, 0);
            
            // Allocate buffer for file data
            char *file_data = malloc(file_size + 1);
            if (!file_data) {
                log_message("Server", "ERROR", "Memory allocation failed");
                continue;
            }
            
            // Receive file data
            int total_received = 0;
            while (total_received < file_size) {
                bytes_received = recv(client_socket, file_data + total_received, file_size - total_received, 0);
                if (bytes_received <= 0) {
                    break;
                }
                total_received += bytes_received;
            }
            
            if (total_received == file_size) {
                file_data[file_size] = '\0';
                handle_decrypt(client_socket, file_data, file_size);
            } else {
                log_message("Server", "ERROR", "Failed to receive complete file data");
            }
            
            free(file_data);
        }
        // Handle DOWNLOAD command
        else if (strncmp(buffer, "DOWNLOAD:", 9) == 0) {
            handle_download(client_socket, buffer + 9);
        }
        // Handle EXIT command
        else if (strncmp(buffer, "EXIT", 4) == 0) {
            log_message("Client", "EXIT", "Client requested to exit");
            break;
        }
        // Handle unknown command
        else {
            char error_msg[] = "ERROR: Unknown command";
            send(client_socket, error_msg, strlen(error_msg), 0);
            log_message("Server", "ERROR", "Unknown command received");
        }
    }
    
    // Log disconnection
    snprintf(log_buffer, sizeof(log_buffer), "Connection closed with %s:%d", client_address, ntohs(addr.sin_port));
    log_message("Server", "DISCONNECT", log_buffer);
    
    close(client_socket);
    return NULL;
}

// Function to create directory if it doesn't exist
void ensure_directory_exists(const char *dir) {
    struct stat st;
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0755);
    }
    
    // Ensure parent directory exists
    char parent_dir[MAX_FILEPATH];
    strcpy(parent_dir, dir);
    char *last_slash = strrchr(parent_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        ensure_directory_exists(parent_dir);
    }
    
    // Create directory
    mkdir(dir, 0755);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t thread_id;
    
    // Ignore SIGPIPE to handle client disconnections gracefully
    signal(SIGPIPE, SIG_IGN);
    
    // Create directories if they don't exist
    ensure_directory_exists(DATABASE_DIR);
    ensure_directory_exists("server");
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server started on port %d\n", PORT);
    char log_buffer[64];
    snprintf(log_buffer, sizeof(log_buffer), "Server started on port %d", PORT);
    log_message("Server", "START", log_buffer);
    
    // Accept connections
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Create thread to handle client
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket;
        
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(client_socket);
            continue;
        }
        
        // Detach thread
        pthread_detach(thread_id);
    }
    
    return 0;
}