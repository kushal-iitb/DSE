#include "TcpHandler.hpp"


namespace DSE :: TcpHandler{

    bool TcpHandler ::setup() {
    memset(&hints , 0 , sizeof(hints)); 
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) !=0){
        std::cerr<<"error in getaddrinfo function"<<std::endl;
        return false;
    }

    for(p = servinfo; p!=NULL; p = p->ai_next){
        if((sockfd = socket(p->ai_family , p->ai_socktype , p->ai_protocol)) == -1){
                std::cerr<<"error in socket function "<<std::endl;
                return false;
        }

        if(setsockopt(sockfd , SOL_SOCKET, SO_REUSEADDR, &optval , sizeof(int)) == -1){
            std::cerr<<"error in setsockopt function "<<std::endl;
            return false;
        }

        if(bind(sockfd, p->ai_addr , p->ai_addrlen) == -1){
            close(sockfd);
            std::cerr<<" error in bind function "<<std::endl;
            return false;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if( p == NULL){
        std::cerr<<" server failed to bind "<<std::endl;
        return false;
    }

    
}

}