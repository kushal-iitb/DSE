#include "TcpHandler.hpp"
#include "logging_object.hpp"
#include "spsc_queue.hpp"

int main(){

    Logger logger;
    logger.init("DSE" , "logs" , 3);

    DSE::spsc::SpscQueue tbt_queue;
    tbt_queue.open("tbt_shm");
    DSE::matching_engine::matchingEngine matchingEngine;
    matchingEngine.set_tbt_queue(&tbt_queue);
    DSE::TcpHandler::TcpHandler tcphandler("5000" , &matchingEngine);
    DSE::TcpHandler::TcpHandler login_handler("9090" ,&matchingEngine);
    if(tcphandler.setup()){
        DSE_LOG_INFO("host is ready to accept connections");
    }
    if(login_handler.setup()){
        DSE_LOG_INFO(" login handler is ready to accept connections");
    }

    tcphandler.start();
    login_handler.start();
    while(true){}
}