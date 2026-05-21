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
#include<unordered_map>

#include "nse_fo_structs.hpp"
#include "matching_engine.hpp"
#include "tls_server.hpp"

namespace DSE :: TcpHandler {

    class TcpHandler{
        public:
        
        TcpHandler(char* PORT, DSE::matching_engine::matchingEngine *matchingEngine , bool tls_mode ,DSE::tls::TlsServer* tls);
        bool setup();
        void stop();
        void send(int fd, char* buffer , size_t len);

        int accept_connections();
        void start();
        void recvdata();
        bool validate_gr_request(DSE::fo::MS_GR_REQUEST& gr_request);
        void send_gr_response(int fd, int32_t traderId);
        void send_secure_box_response(int fd, int32_t traderId);
        void send_box_signon_response(int fd , int32_t traderId);
        void send_signon_response(int fd, int32_t traderId);
        private:
        std::thread tcp_connection_thread,tcp_recv_thread;
        std::vector<int> connection_fds;
        std::mutex mt;
        int sockfd{-1}, newfd{-1} , rv{0};
        char* PORT = "5000";
        int optval = 1;
        struct addrinfo hints , *servinfo , *p;
        std::unordered_map<int32_t , int16_t> TraderIdbyBoxId;
        DSE::matching_engine::matchingEngine* matchingEngine;
        DSE::tls::TlsServer* tls = nullptr;
        bool tls_mode = false;
        std::unordered_map<int , SSL*> SSLfd;
};

}