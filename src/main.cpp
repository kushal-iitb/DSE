#include "TcpHandler.hpp"
#include "logging_object.hpp"

int main(){

    Logger logger;
    logger.init("DSE" , "logs" , 3);


    DSE::TcpHandler::TcpHandler tcphandler;
    if(tcphandler.setup()){
        DSE_LOG_INFO("host is ready to accept connections");
    }

    tcphandler.start();
    while(true){}
}