#pragma once

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<iostream>
#include<string.h>
#include <unistd.h>

namespace DSE :: TcpHandler {

    class TcpHandler{
        public:
        
        bool setup();
        void stop();
        void send();
        void recv();

        private:
        int sockfd , newfd , rv;
        const char* PORT = "5000";
        int optval = 1;
        struct addrinfo hints , *servinfo , *p;
};

}