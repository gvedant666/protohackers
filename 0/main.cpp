#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>

void handle_client(int client_socket){
    // 4096 is a much more standard buffer size for TCP packets
    char buffer[4096];
    
    while(true){
        // FIX: Read the FULL size of the buffer. We treat this as raw binary data, not strings.
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) {
            break;
        }
        if (bytes_read == 0) {
            // Client gracefully closed connection
            break;
        }

        ssize_t total_write = 0;
        while(total_write < bytes_read) {
            ssize_t result = write(client_socket, buffer + total_write, bytes_read - total_write);
            if (result < 0) {
                close(client_socket);
                return;
            }
            total_write += result;
        }
    }

    close(client_socket);
}

int main() {
    int server_socket;
    struct sockaddr_in6 server_addr;

    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket < 0) { // Sockets return -1 on failure
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    struct sockaddr_in6 server_addr; // Notice the '6'
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any; // Listen on all interfaces, including IPv6
    server_addr.sin6_port = htons(8080);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    if (listen(server_socket, 50) < 0) { // Increased backlog for Protohackers concurrency
        std::cerr << "Failed to listen on socket" << std::endl;
        return 1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;

    while (true) {
        // FIX: Create an isolated structure for the incoming client data
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Pass the client_addr here, leaving server_addr untouched!
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            continue;
        }

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_socket);
    return 0;
}