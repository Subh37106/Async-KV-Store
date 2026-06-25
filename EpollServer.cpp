#include "EpollServer.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>

EpollServer::EpollServer() {
    setup_server();
}

EpollServer::~EpollServer() {
    close(server_fd);
    close(epoll_fd);
}

void EpollServer::make_socket_non_blocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

void EpollServer::setup_server() {
    // 1. Create a TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind the socket to PORT 8080
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    // 3. Listen for incoming connections
    listen(server_fd, SOMAXCONN);
    make_socket_non_blocking(server_fd);

    // 4. Create the epoll instance
    epoll_fd = epoll_create1(0);

    // 5. Add the server socket to epoll to monitor for incoming connections (EPOLLIN)
    struct epoll_event event{};
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET; // EPOLLET = Edge Triggered mode (very fast)
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
}

void EpollServer::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Accept all incoming connections waiting in the queue
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            break; // No more connections to accept right now
        }

        make_socket_non_blocking(client_fd);
        struct epoll_event event{};
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
    }
}

void EpollServer::handle_client_data(int client_fd) {
    char buffer[1024];
    std::string request_data;

    while (true) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            request_data += buffer;
        } else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // We have read all available data
            break;
        } else {
            // Client disconnected or error
            close(client_fd);
            return;
        }
    }

    if (!request_data.empty()) {
        process_command(client_fd, request_data);
    }
}

void EpollServer::process_command(int client_fd, const std::string& data) {
    std::istringstream iss(data);
    std::string command, key, value;
    iss >> command >> key;

    std::string response;
    if (command == "SET") {
        std::getline(iss, value);
        // Trim leading space
        if (!value.empty() && value[0] == ' ') value = value.substr(1);
        db.set(key, value);
        response = "OK\n";
    } else if (command == "GET") {
        response = db.get(key) + "\n";
    } else if (command == "DEL") {
        db.del(key);
        response = "OK\n";
    } else {
        response = "ERROR: Unknown command\n";
    }

    // Write the response back to the client
   
ssize_t bytes_written = write(client_fd, response.c_str(), response.length());
if (bytes_written < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cerr << "Error writing to client " << client_fd << "\n";
    }
}
}

void EpollServer::start() {
    std::cout << "Epoll KV Server running on port " << PORT << "...\n";
    
    while (true) {
        // Wait for network activity (blocks until data arrives)
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd) {
                // Someone is trying to connect
                handle_new_connection();
            } else {
                // An existing client sent us data
                handle_client_data(events[i].data.fd);
            }
        }
    }
}