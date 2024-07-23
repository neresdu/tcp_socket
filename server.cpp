#include <iostream>
#include <fstream>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>

#define PORT 8080
#define BUFFER_SIZE 4096

// Helper function to generate a unique filename
std::string generate_filename() {
    auto now = std::chrono::high_resolution_clock::now();
    auto now_c = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    std::ostringstream oss;
    oss << "received_file_" << now_c << ".mf4";
    return oss.str();
}

void handle_client(int client_socket) {
    char header[16];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, header, 16, 0)) > 0) {
        // Read the file size from the header
        header[bytes_received] = '\0';  // Null-terminate header
        std::istringstream iss(header);
        std::size_t file_size;
        iss >> file_size;

        std::string filename = generate_filename();
        std::ofstream output_file(filename, std::ios::binary);
        if (!output_file) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return;
        }

        std::cout << "Receiving file: " << filename << " (" << file_size << " bytes)" << std::endl;

        // Receive file data
        char buffer[BUFFER_SIZE];
        std::size_t total_received = 0;
        while (total_received < file_size && (bytes_received = recv(client_socket, buffer, std::min(BUFFER_SIZE, static_cast<int>(file_size - total_received)), 0)) > 0) {
            output_file.write(buffer, bytes_received);
            total_received += bytes_received;
        }

        output_file.close();
        std::cout << "File " << filename << " received successfully." << std::endl;
    }

    close(client_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))) {
        std::cout << "Connection accepted." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        handle_client(new_socket);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "File handling completed in " << elapsed.count() << " seconds." << std::endl;
    }

    close(server_fd);
    return 0;
}
