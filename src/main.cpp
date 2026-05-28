#include "TcpHandler.hpp"
#include "logging_object.hpp"
#include "spsc_queue.hpp"
#include "tls_server.hpp"

#include <atomic>
#include <csignal>
#include <chrono>
#include <thread>

static std::atomic<bool> g_shutdown{false};

static void on_signal(int){
    g_shutdown.store(true, std::memory_order_release);
}

int main(){

    Logger logger;
    logger.init("DSE" , "logs" , 3);

    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    DSE::spsc::SpscQueue tbt_queue;
    tbt_queue.open("tbt_shm");
    DSE::matching_engine::matchingEngine matchingEngine;
    matchingEngine.set_tbt_queue(&tbt_queue);
    DSE::tls::TlsServer tls;
    if(!tls.init("certs/dse.crt" , "certs/dse.key")) return 1;

    DSE::TcpHandler::TcpHandler tcphandler("5000" , &matchingEngine , true , &tls);
    DSE::TcpHandler::TcpHandler login_handler("9090" ,&matchingEngine , false , nullptr);

    if(!tcphandler.setup()){
        DSE_LOG_ERROR("GR handler setup failed (port 5000 in use?)");
        return 1;
    }
    DSE_LOG_INFO("host is ready to accept connections");

    if(!login_handler.setup()){
        DSE_LOG_ERROR("login handler setup failed (port 9090 in use?)");
        tcphandler.stop();
        return 1;
    }
    DSE_LOG_INFO(" login handler is ready to accept connections");

    tcphandler.start();
    login_handler.start();

    while(!g_shutdown.load(std::memory_order_acquire)){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    DSE_LOG_INFO("shutdown signal received — stopping handlers");
    tcphandler.stop();
    login_handler.stop();
    DSE_LOG_INFO("clean exit");
    return 0;
}
