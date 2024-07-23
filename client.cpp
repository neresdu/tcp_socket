#include <iostream>
#include <fstream>
#include <chrono>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

std::vector<std::string> get_files_in_directory(const std::string &directory) {
    std::vector<std::string> file_list;
    struct dirent *entry;
    DIR *dp = opendir(directory.c_str());

    if (dp == nullptr) {
        perror("opendir");
        return file_list;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) { // Regular file
            std::string filename(entry->d_name);
            if (filename.find(".mf4") != std::string::npos) {
                file_list.push_back(directory + "/" + filename);
            }
        }
    }

    closedir(dp);
    return file_list;
}

void send_file(int sock, const std::string &file_path) {
    std::ifstream input_file(file_path, std::ios::binary | std::ios::ate);
    if (!input_file) {
        std::cerr << "Failed to open file for reading: " << file_path << std::endl;
        return;
    }

    std::streamsize file_size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    // Send file size header
    std::string header = std::to_string(file_size);
    header.resize(16, ' '); // Pad header to 16 bytes
    send(sock, header.c_str(), 16, 0);

    char buffer[BUFFER_SIZE];
    while (input_file.read(buffer, BUFFER_SIZE) || input_file.gcount() > 0) {
        send(sock, buffer, input_file.gcount(), 0);
    }

    input_file.close();
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    std::string directory = "./mf4_files"; // Change this to your directory path

    std::vector<std::string> files = get_files_in_directory(directory);
    if (files.empty()) {
        std::cerr << "No .mf4 files found in the directory." << std::endl;
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    for (const auto &file_path : files) {
        std::cout << "Sending file: " << file_path << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        send_file(sock, file_path);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "File sent in " << elapsed.count() << " seconds." << std::endl;
    }

    close(sock);
    return 0;
}
