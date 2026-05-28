#include "TcpHandler.hpp"
#include "logging_object.hpp"
#include "bswap.hpp"
#include "nse_fo_structs.hpp"
#include "tls_server.hpp"

namespace DSE :: TcpHandler{

    TcpHandler::TcpHandler(char* port , DSE::matching_engine::matchingEngine* matchingEngine ,bool tls_mode , DSE::tls::TlsServer* tls){
        this->PORT = port;
        this->matchingEngine = matchingEngine;
        this->tls = tls;
        this->tls_mode = tls_mode;
    }

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
        
        if(tls_mode){
            SSL* ssl = tls->accept(connection);
            if(!ssl){
                ::close(connection);
                DSE_LOG_ERROR(" error in setting up ssl connection on fd = {}" , connection);
                return -1;
            }
            else{
                SSLfd[connection] =ssl;
            }
        }

        int flags = fcntl(connection , F_GETFL , 0);
        fcntl(connection , F_SETFL , flags | O_NONBLOCK);

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
        DSE_LOG_INFO(" seding GR Response");
        this->send(fd, buffer , total_len);

    }

    void TcpHandler::send_secure_box_response(int fd, int32_t traderId){
        int16_t boxId = TraderIdbyBoxId[traderId];
        DSE::fo::SECURE_BOX_REGISTRATION_RESPONSE sb_response;
        sb_response.Header.TransactionCode = DSE::bswap::bswap16(23009);
        sb_response.Header.MessageLength = DSE::bswap::bswap16(sizeof(DSE::fo::SECURE_BOX_REGISTRATION_RESPONSE));
        sb_response.Header.TraderId = DSE::bswap::bswap32(traderId);
       
        constexpr size_t total_len = sizeof(DSE::fo::WireHeader) + sizeof(DSE::fo::SECURE_BOX_REGISTRATION_RESPONSE);
        char buffer[total_len];
        auto* wire_header = reinterpret_cast<DSE::fo::WireHeader*>(buffer);
        wire_header->packet_length = DSE::bswap::bswap16(total_len);
        wire_header->sequence_number = DSE::bswap::bswap32(3);
        memset(wire_header->checksum , 0 , 16);
        std::memcpy(buffer+sizeof(DSE::fo::WireHeader) , &sb_response , sizeof(sb_response));
        DSE_LOG_INFO("sending secure box response ");
        this->send(fd, buffer , total_len);
    }

    void TcpHandler::send_box_signon_response(int fd , int32_t traderId){
        int16_t boxId = TraderIdbyBoxId[traderId];
        DSE::fo::BOX_SIGN_ON_RESPONSE box_sign_on_response;
        box_sign_on_response.Header.TransactionCode = DSE::bswap::bswap16(23001);
        box_sign_on_response.Header.MessageLength = DSE::bswap::bswap16(sizeof(DSE::fo::BOX_SIGN_ON_RESPONSE));
        box_sign_on_response.Header.TraderId = DSE::bswap::bswap32(traderId);

        box_sign_on_response.BoxId = DSE::bswap::bswap16(boxId);

        memset(box_sign_on_response.Reserved , 0 , sizeof(box_sign_on_response.Reserved));
       
        constexpr size_t total_len = sizeof(DSE::fo::WireHeader) + sizeof(DSE::fo::BOX_SIGN_ON_RESPONSE);
        char buffer[total_len];
        auto* wire_header = reinterpret_cast<DSE::fo::WireHeader*>(buffer);
        wire_header->packet_length = DSE::bswap::bswap16(total_len);
        wire_header->sequence_number = DSE::bswap::bswap32(4);
        memset(wire_header->checksum , 0 , 16);
        std::memcpy(buffer+sizeof(DSE::fo::WireHeader) , &box_sign_on_response , sizeof(box_sign_on_response));
        DSE_LOG_INFO("sending box sign on response ");
        this->send(fd, buffer , total_len);
    }

    void TcpHandler::send_signon_response(int fd, int32_t traderId){
    DSE::fo::MS_SIGNON signon_response{};   
    signon_response.Header.TransactionCode = DSE::bswap::bswap16(2301);
    signon_response.Header.MessageLength   = DSE::bswap::bswap16(sizeof(DSE::fo::MS_SIGNON));
    signon_response.Header.TraderId        = DSE::bswap::bswap32(traderId);

    signon_response.UserID = DSE::bswap::bswap32(traderId);
    std::memcpy(signon_response.TraderName, "DSE_TEST_TRADER           ", 26);
    std::memcpy(signon_response.BrokerID,   "TM001", 5);
    signon_response.BranchID       = DSE::bswap::bswap16(1);
    signon_response.VersionNumber  = DSE::bswap::bswap32(94600);   // protocol 9.46
    signon_response.Batch2StartTime= DSE::bswap::bswap32(0);
    signon_response.UserType       = DSE::bswap::bswap16(0);       // 0 = dealer
    signon_response.SequenceNumber = DSE::bswap::bswap64(1);
    std::memcpy(signon_response.BrokerName, "DSE_TEST_BROKER_NAME     ", 25);
    signon_response.MemberType     = DSE::bswap::bswap16(0);
    signon_response.BrokerStatus   = 'A';        // Active
    signon_response.ClearingStatus = 'C';        // Clearing
    signon_response.ShowIndex      = 'T';        // T = show, F = hide

    constexpr size_t total_len = sizeof(DSE::fo::WireHeader) + sizeof(DSE::fo::MS_SIGNON);
    char buffer[total_len];
    auto* wire_header = reinterpret_cast<DSE::fo::WireHeader*>(buffer);
    wire_header->packet_length   = DSE::bswap::bswap16(total_len);
    wire_header->sequence_number = DSE::bswap::bswap32(5);
    std::memset(wire_header->checksum, 0, 16);
    std::memcpy(buffer + sizeof(DSE::fo::WireHeader), &signon_response, sizeof(signon_response));

    DSE_LOG_INFO("sending MS_SIGNON response (tcode=2301)");
    this->send_encrypted(fd, buffer, total_len);
}


    void TcpHandler::recvdata(){
        if(connection_fds.empty())
        return;
        std::lock_guard<std::mutex> lock(mt);
        for(auto it : connection_fds){
        char wire_header[22] = {0};
        
        int data = tls_mode ? SSL_read(SSLfd[it] ,wire_header , sizeof(wire_header)) : recv(it , wire_header , sizeof(wire_header) , MSG_DONTWAIT);
        
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
                size_t body_len = packet_length - sizeof(DSE::fo::WireHeader);
                char recv_buffer[2048] = {0};
                if(body_len > sizeof(recv_buffer))
                continue;
                size_t got = 0;
                int retry = 1000;
                while(got < body_len && retry-- >0){
                data = tls_mode
                    ? SSL_read(SSLfd[it], recv_buffer +got, body_len - got)
                    : recv(it, recv_buffer + got, body_len - got, MSG_DONTWAIT);
                    if(data>0)
                    got+=data;
                    else if(data<0 && (errno == EAGAIN  || errno == EWOULDBLOCK)){
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                    else{
                        break;
                    }
                } 
                data = (int)got;                   
                     if(!decrypt_in_place(it , wire_header , recv_buffer, (size_t)data))
                        continue;
                        if(data==sizeof(DSE::fo::MS_GR_REQUEST)){
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
                        else if(data == sizeof(DSE::fo::SECURE_BOX_REGISTRATION_REQUEST)){
                            auto* sb_request = reinterpret_cast<DSE::fo::SECURE_BOX_REGISTRATION_REQUEST*>(recv_buffer);
                            uint8_t computed[16];
                            MD5(reinterpret_cast<const uint8_t*>(sb_request) , sizeof(*sb_request) , computed);
                            if(std::memcmp(computed , hdr->checksum , 16) !=0){
                                DSE_LOG_ERROR(" MD5 mismatch on SECURE_BOX_REG");
                                return;
                            }
			    DSE_LOG_INFO(" MD5 verified for SECURE_BOX_REG");
                            uint16_t tcode = DSE::bswap::bswap16(sb_request->Header.TransactionCode);
                            uint32_t trader = DSE::bswap::bswap32(sb_request->Header.TraderId);
                            DSE_LOG_INFO(" Message Header received , transcation code = {} , traderId = {} " , tcode , trader);

                            send_secure_box_response(it , trader);

                        }
                        else if(data == sizeof(DSE::fo::BOX_SIGN_ON_REQUEST)){
                            auto* box_sign_on_request = reinterpret_cast<DSE::fo::BOX_SIGN_ON_REQUEST*>(recv_buffer);
                            uint16_t tcode = DSE::bswap::bswap16(box_sign_on_request->Header.TransactionCode);
                            uint32_t trader = DSE::bswap::bswap32(box_sign_on_request->Header.TraderId);
                            DSE_LOG_INFO(" Message Header received , transcation code = {} , traderId = {} " , tcode , trader);

                            auto box = std::make_unique<DSE::crypto::BoxSession>();
                            unsigned char key[32] , iv[12];
                            std::memset(key , 0x55 , sizeof(key));
                            std::memset(iv , 0x77 , sizeof(iv));

                            if(!box->init(key ,iv)){
                                DSE_LOG_ERROR(" BoxSession init failed for fd = {} , " , it);
                            }
                            else{
                                DSE_LOG_INFO(" BoxSession initialised for fd = {} " , it);
                                sessions[it] = std::move(box);
                            }

                            send_box_signon_response(it , trader);
                        }
                        else if(data == sizeof(DSE::fo::MS_SIGNON)){
                            auto* signon_request = reinterpret_cast<DSE::fo::MS_SIGNON*>(recv_buffer);
                            uint16_t tcode  = DSE::bswap::bswap16(signon_request->Header.TransactionCode);
                            uint32_t trader = DSE::bswap::bswap32(signon_request->Header.TraderId);
                            int32_t userId  = DSE::bswap::bswap32(signon_request->UserID);
                            DSE_LOG_INFO(" Message Header received , transcation code = {} , traderId = {} , userId = {} ",
                            tcode, trader, userId);

                        send_signon_response(it, trader);
                        }
                        else if(data == sizeof(DSE::fo::MS_OE_REQUEST_TR)){
                            auto* oe_request = reinterpret_cast<DSE::fo::MS_OE_REQUEST_TR*>(recv_buffer);
                            uint16_t tcode  = DSE::bswap::bswap16(oe_request->TransactionCode);
                            int32_t  token  = DSE::bswap::bswap32(oe_request->TokenNo);
                            int32_t  price  = DSE::bswap::bswap32(oe_request->Price);
                            int32_t  vol    = DSE::bswap::bswap32(oe_request->Volume);
                            int16_t  side   = DSE::bswap::bswap16(oe_request->BuySellIndicator);
                            int32_t  trader = DSE::bswap::bswap32(oe_request->TraderId);
                            DSE_LOG_INFO(" NEW_ORDER received , tcode = {} , token = {} , price = {} , qty = {} , side = {} , traderId = {} ",
                                         tcode, token, price, vol, side, trader);
                            matchingEngine->onNewOrder(*oe_request);
                        }
                        else if(data == sizeof(DSE::fo::MS_OM_REQUEST_TR)){
                            auto* om_request = reinterpret_cast<DSE::fo::MS_OM_REQUEST_TR*>(recv_buffer);
                            uint16_t tcode  = DSE::bswap::bswap16(om_request->TransactionCode);
                            int32_t  token  = DSE::bswap::bswap32(om_request->TokenNo);
                            int32_t  price  = DSE::bswap::bswap32(om_request->Price);
                            int32_t  vol    = DSE::bswap::bswap32(om_request->Volume);
                            int16_t  side   = DSE::bswap::bswap16(om_request->BuySellIndicator);
                            int32_t  trader = DSE::bswap::bswap32(om_request->TraderId);
                            uint64_t _raw_on;
                            std::memcpy(&_raw_on, &om_request->OrderNumber, 8);
                            uint32_t orderId = (uint32_t)be64toh(_raw_on);
                            const char* op = (tcode == 20040) ? "MODIFY" : "CANCEL";
                            DSE_LOG_INFO(" {} received , tcode = {} , orderId = {} , token = {} , price = {} , qty = {} , side = {} , traderId = {} ",
                                         op, tcode, orderId, token, price, vol, side, trader);
                            if(tcode == 20040){
                                matchingEngine->onModifyOrder(*om_request);
                            }
                            else{
                                matchingEngine->onCancelOrder(*om_request);
                            }
                        }

                        else{
                            DSE_LOG_ERROR(" expected 48 bytes of data for GR_REQUEST");
                        }
            }
        }
    }
    }

    void TcpHandler::send_encrypted(int fd , char* buffer, size_t total_len){
        auto box_session = sessions.find(fd);
        if( box_session == sessions.end() || ! box_session->second){
            this->send(fd , buffer , total_len);
            return;
        }

        auto* wire = reinterpret_cast<DSE::fo::WireHeader*>(buffer);
        unsigned char* pt = reinterpret_cast<unsigned char*>(buffer + sizeof(DSE::fo::WireHeader));
        size_t plain_len = total_len - sizeof(DSE::fo::WireHeader);

        unsigned char tag[16];
        unsigned char ct[1024];
        
        if(! box_session->second->encrypt(pt , plain_len , nullptr , 0 , ct , tag)){
            DSE_LOG_ERROR( " encrypt failed for fd  = {} " , fd);
            return;
        }
        std::memcpy(pt , ct , plain_len);
        std::memcpy(wire->checksum , tag , 16);
        this->send(fd , buffer, total_len);
    }

    bool TcpHandler::decrypt_in_place(int fd , char* wire_header , char* body , size_t body_len){
        auto box_session = sessions.find(fd);
        if(box_session == sessions.end() || ! box_session->second){
            return true;
        }

        auto* wire = reinterpret_cast<DSE::fo::WireHeader*>(wire_header);
        unsigned char* ct = reinterpret_cast<unsigned char*>(body);
        unsigned char pt[1024];

        if(! box_session->second->decrypt(ct , body_len , nullptr, 0 , reinterpret_cast<unsigned char*>(wire->checksum) , pt)){
            DSE_LOG_ERROR(" decrypt failed for fd = {} ", fd);
            return false;
        }
        std::memcpy(ct , pt , body_len);
        return true;
    }

    void TcpHandler::start(){
        if(sockfd == -1){
            DSE_LOG_ERROR(" start() called but setup() failed — refusing to launch threads");
            return;
        }
        running_.store(true, std::memory_order_release);

        tcp_connection_thread = std::thread([this]{
            while(running_.load(std::memory_order_acquire)){
                int conn = accept_connections();
                if(conn != -1){
                    DSE_LOG_INFO(" connection accepted");
                    std::lock_guard<std::mutex> lock(mt);
                    connection_fds.push_back(conn);
                }
            }
        });

        tcp_recv_thread = std::thread([this](){
            while(running_.load(std::memory_order_acquire)){
                recvdata();
            }
        });
    }

    void TcpHandler::stop(){
        running_.store(false, std::memory_order_release);
        if(sockfd != -1){
            ::shutdown(sockfd, SHUT_RDWR);
            ::close(sockfd);
            sockfd = -1;
        }
        if(tcp_connection_thread.joinable()) tcp_connection_thread.join();
        if(tcp_recv_thread.joinable())       tcp_recv_thread.join();

        std::lock_guard<std::mutex> lock(mt);
        for(int fd : connection_fds){
            auto sit = SSLfd.find(fd);
            if(sit != SSLfd.end() && tls) tls->shutdown(sit->second);
            ::close(fd);
        }
        connection_fds.clear();
        SSLfd.clear();
        sessions.clear();
    }

    void TcpHandler::send(int fd , char* buffer , size_t len){

       tls_mode ? SSL_write(SSLfd[fd] , buffer, len) :  ::send(fd, buffer, len , 0);

    }

 
}
