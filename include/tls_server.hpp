#pragma once

#include<openssl/ssl.h>
#include<openssl/err.h>
#include<openssl/md5.h>

namespace DSE::tls{

    class TlsServer{
        private:
        SSL_CTX* ctx = nullptr;

        public:

        bool init(const char* cert_path , const char* key_path );
        SSL* accept(int fd);
        void shutdown(SSL* ssl);
        void close();
    };
} // namespace DSE::tls