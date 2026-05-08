#pragma once

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<iostream>
#include<string.h>
#include <unistd.h>
#include<thread>
#include<mutex>
#include<vector>



namespace DSE :: TcpHandler {

    class TcpHandler{
        public:
        
        bool setup();
        void stop();
        void send();

        int accept_connections();
        void start();
        void recvdata();

        private:
        std::thread tcp_connection_thread,tcp_recv_thread;
        std::vector<int> connection_fds;
        std::mutex mt;
        int sockfd{-1}, newfd{-1} , rv{0};
        const char* PORT = "5000";
        int optval = 1;
        struct addrinfo hints , *servinfo , *p;
};

}