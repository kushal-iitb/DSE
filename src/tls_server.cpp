#include"tls_server.hpp"
#include"logging_object.hpp"

namespace DSE::tls{

    bool DSE::tls::TlsServer::init(const char* cert_path , const char* key_path){
        SSL_library_init();
        OpenSSL_add_ssl_algorithms();
        SSL_load_error_strings();

        ctx = SSL_CTX_new(TLS_server_method());
        if(!ctx){
            DSE_LOG_ERROR(" SSL_CTX_new failed");
            return false;
        }

        SSL_CTX_set_min_proto_version(ctx , TLS1_3_VERSION);
        SSL_CTX_set_max_proto_version(ctx , TLS1_3_VERSION);

        SSL_CTX_set_ciphersuites(ctx , "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");

        if(SSL_CTX_use_certificate_file(ctx , cert_path , SSL_FILETYPE_PEM) != 1){
        DSE_LOG_ERROR("cert load failed: {}", cert_path);
        return false;
        }

        if(SSL_CTX_use_PrivateKey_file(ctx , key_path , SSL_FILETYPE_PEM) != 1){
        DSE_LOG_ERROR("key load failed: {}", key_path);
        return false;
        }

        if(SSL_CTX_check_private_key(ctx) != 1){
        DSE_LOG_ERROR("cert/key mismatch");
        return false;
        }

        return true;

    }

    SSL* DSE::tls::TlsServer::accept(int fd){
        SSL* ssl = SSL_new(ctx);
        if(!ssl){
            return nullptr;
        }
        SSL_set_fd(ssl , fd);

        int rc = SSL_accept(ssl);

        if(rc <=0){
            int err = SSL_get_error(ssl , rc);
            DSE_LOG_ERROR("SSL_accept failed : err = {}" , err);
            SSL_free(ssl);
            return nullptr;
        }

        return ssl;
    }

    void DSE::tls::TlsServer::shutdown(SSL* ssl){
        if(!ssl)
        return;
        SSL_shutdown(ssl);
        SSL_free(ssl);
    } 


    void DSE::tls::TlsServer::close(){
        if(ctx){
            SSL_CTX_free(ctx);
            ctx=nullptr;
        }
    }


}