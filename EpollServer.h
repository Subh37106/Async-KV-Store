#pragma once
#include "Database.h"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <vector>

constexpr int MAX_EVENTS = 10000;
constexpr int PORT = 8080;

class EpollServer {
public:
    EpollServer();
    ~EpollServer();
    void start();

private:
    int server_fd;
    int epoll_fd;
    Database db;
    struct epoll_event events[MAX_EVENTS];

    void setup_server();
    void make_socket_non_blocking(int socket_fd);
    void handle_new_connection();
    void handle_client_data(int client_fd);
    
    // A simple protocol parser: SET key value, GET key, DEL key
    void process_command(int client_fd, const std::string& command);
};