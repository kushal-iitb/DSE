#include "TcpHandler.hpp"
#include "logging_object.hpp"
#include "bswap.hpp"
#include "nse_fo_structs.hpp"

namespace DSE :: TcpHandler{

    bool TcpHandler ::setup() {
    memset(&hints , 0 , sizeof(hints)); 
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    TraderIdbyBoxId[12345] = 100;   // TO-DO need to remove this, added this for testing purpose only

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

    bool TcpHandler::validate_gr_request(DSE::fo::MS_GR_REQUEST& gr_request){
        int32_t traderId = DSE::bswap::bswap32(gr_request.header.TraderId);
        if(TraderIdbyBoxId.find(traderId) != TraderIdbyBoxId.end()){
            if(TraderIdbyBoxId[traderId] == DSE::bswap::bswap16(gr_request.BoxId))
            return true;
        }
        return false;
    }

     void TcpHandler::send_gr_response(int fd, int32_t traderId){
        int16_t boxId = TraderIdbyBoxId[traderId];
        DSE::fo::MS_GR_RESPONSE gr_response;
        gr_response.Header.TransactionCode = DSE::bswap::bswap16(2401);
        gr_response.Header.MessageLength = DSE::bswap::bswap16(sizeof(DSE::fo::MS_GR_RESPONSE));
        gr_response.Header.TraderId = DSE::bswap::bswap32(traderId);
        gr_response.BoxId = DSE::bswap::bswap16(boxId);
        std::memcpy(gr_response.BrokerId , "TM001" , 5);
        gr_response.Filler = '0';
        std::memset(gr_response.IPAddress , 0 , sizeof(gr_response.IPAddress));
        std::memcpy(gr_response.IPAddress , "127.0.0.1" , sizeof(gr_response.IPAddress));
        gr_response.Port = DSE::bswap::bswap32(9090);
        std::memset(gr_response.SessionKey , 0x42 , 8);
        std::memset(gr_response.CryptographicKey , 0x55 , 32);
        std::memset(gr_response.CryptographicIV , 0x77 , 16);

        constexpr size_t total_len = sizeof(DSE::fo::WireHeader) + sizeof(DSE::fo::MS_GR_RESPONSE);
        char buffer[total_len];
        auto* wire_header = reinterpret_cast<DSE::fo::WireHeader*>(buffer);
        wire_header->packet_length = DSE::bswap::bswap16(total_len);
        wire_header->sequence_number = DSE::bswap::bswap32(2);
        memset(wire_header->checksum , 0 , 16);
        std::memcpy(buffer+sizeof(DSE::fo::WireHeader) , &gr_response , sizeof(gr_response));

        this->send(fd, buffer , total_len);

    }


    void TcpHandler::recvdata(){
        if(connection_fds.empty())
        return;
        std::lock_guard<std::mutex> lock(mt);
        for(auto it : connection_fds){
        char wire_header[22] = {0};
        int data = recv(it , wire_header , sizeof(wire_header) , MSG_DONTWAIT);
        if(data == 22){
            DSE::fo::WireHeader* hdr = reinterpret_cast<DSE::fo::WireHeader*>(wire_header);
            uint16_t packet_length = DSE::bswap::bswap16(hdr->packet_length);
            uint32_t sequence_no = DSE::bswap::bswap32(hdr->sequence_number);

            // char checksum_hex[33] = {0};   // 16 bytes * 2 hex chars + NUL
            // for (int i = 0; i < 16; i++) {
            //     snprintf(checksum_hex + i*2, 3, "%02x", (uint8_t)hdr->checksum[i]);
            // }

             DSE_LOG_INFO(" received wire header , packet_length  = {}  , sequence_no = {} , checksum = {} " , packet_length , sequence_no ,hdr->checksum );
            if(packet_length > sizeof(DSE::fo::WireHeader)){
                uint16_t body_len = packet_length - sizeof(DSE::fo::WireHeader);
                char recv_buffer[1024] = {0};
                data = recv(it , recv_buffer , sizeof(recv_buffer) , MSG_DONTWAIT);
                        if(data==body_len){
                        DSE_LOG_INFO( " received data from connection_id = {} , data = {} " , it , recv_buffer);
                        auto* message_header = reinterpret_cast<DSE::fo::Message_Header*>(recv_buffer);
                        uint16_t tcode = DSE::bswap::bswap16(message_header->TransactionCode);
                        uint32_t trader = DSE::bswap::bswap32(message_header->TraderId);
                        uint16_t mlen = DSE::bswap::bswap16(message_header->MessageLength);
                        DSE_LOG_INFO(" Message Header received , transaction code = {} , traderId = {} , message length = {} ", tcode , trader , mlen);

                        if(tcode == 2400 && data >= (int)sizeof(DSE::fo::MS_GR_REQUEST)){
                            auto* gr_request = reinterpret_cast<DSE::fo::MS_GR_REQUEST*>(recv_buffer);
                            uint16_t boxId = DSE::bswap::bswap16(gr_request->BoxId);
                            DSE_LOG_INFO(" GR_REQUEST received , bodId = {} , brokerId = {}" , boxId, gr_request->BrokerId);
                            if(validate_gr_request(*gr_request)){
                                send_gr_response(it, trader);
                            }
                            else{
                                DSE_LOG_ERROR(" no mapping found for TraderId = {} , with BoxId = {}" , trader , boxId);
                            }
                        }
                        }
                        else{
                            DSE_LOG_ERROR(" expected 48 bytes of data for GR_REQUEST");
                        }
            }
        }
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

    void TcpHandler::send(int fd , char* buffer , size_t len){

        ::send(fd, buffer, len , 0);

    }

 
}