#include "TcpHandler.hpp"
#include "logging_object.hpp"

namespace DSE :: TcpHandler{

    bool TcpHandler ::setup() {
    memset(&hints , 0 , sizeof(hints)); 
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) !=0){
        DSE_LOG_ERROR("error in getaddrinfo function");
        return false;
    }

    for(p = servinfo; p!=NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family , p->ai_socktype , p->ai_protocol)) == -1){
                DSE_LOG_ERROR("error in socket function");
                freeaddrinfo(servinfo);
                return false;
        }

        if(setsockopt(sockfd , SOL_SOCKET, SO_REUSEADDR, &optval , sizeof(int)) == -1){
            DSE_LOG_ERROR("error in setsockopt function");
            freeaddrinfo(servinfo);
            return false;
        }

        if(bind(sockfd, p->ai_addr , p->ai_addrlen) == -1){
            DSE_LOG_ERROR("error in bind function");
            close(sockfd);
            sockfd = -1;
            freeaddrinfo(servinfo);
            return false;
        }

        if(listen(sockfd, SOMAXCONN) == -1){
            DSE_LOG_ERROR("error in listen function");
            close(sockfd);
            sockfd = -1;
            freeaddrinfo(servinfo);
            return false;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if( p == NULL){
        DSE_LOG_INFO("server failed to bind");
        return false;
    }

    return true;
}

    int TcpHandler::accept_connections(){
        
        sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int connection = accept(sockfd, (sockaddr*)&client_addr , &addr_size);
        if(connection == -1){
            DSE_LOG_ERROR("error in accept function , errno = {}" , errno);
            return -1;
        }
        return connection;
    }

    void TcpHandler::recvdata(){
        if(connection_fds.empty())
        return;
        std::lock_guard<std::mutex> lock(mt);
        for(auto it : connection_fds){
        char buffer_recv[1024] = {0};
        int data = recv(it , buffer_recv , sizeof(buffer_recv) , MSG_DONTWAIT);
        if(data>0)
        DSE_LOG_INFO( " received data from connection_id = {} , data = {} " , it , buffer_recv);
        }

    }

    void TcpHandler::start(){
        tcp_connection_thread  = std::thread([this]{
            while(true){
            int conn = accept_connections();
            if(conn!=-1){
            DSE_LOG_INFO(" connection accepted");
            std::lock_guard<std::mutex> lock(mt);
            connection_fds.push_back(conn);
            }
            // std::this_thread::sleep_for(std::chrono::seconds(15));

            }
    });

    tcp_recv_thread  = std::thread([this](){
        while(true){
            recvdata();
        }
    });

        tcp_connection_thread.detach();
        tcp_recv_thread.detach();
    }

    void TcpHandler::stop(){

    }

    void TcpHandler::send(){

    }

 
}