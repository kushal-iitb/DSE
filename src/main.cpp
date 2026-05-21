#include "TcpHandler.hpp"
#include "logging_object.hpp"
#include "spsc_queue.hpp"
#include "tls_server.hpp"

int main(){

    Logger logger;
    logger.init("DSE" , "logs" , 3);

    DSE::spsc::SpscQueue tbt_queue;
    tbt_queue.open("tbt_shm");
    DSE::matching_engine::matchingEngine matchingEngine;
    matchingEngine.set_tbt_queue(&tbt_queue);
    DSE::tls::TlsServer tls;
    if(!tls.init("certs/dse.crt" , "certs/dse.key")) return 1;
    DSE::TcpHandler::TcpHandler tcphandler("5000" , &matchingEngine , true , &tls);
    DSE::TcpHandler::TcpHandler login_handler("9090" ,&matchingEngine , false , nullptr);
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