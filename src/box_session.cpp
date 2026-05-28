#include "box_session.hpp"
#include "logging_object.hpp"

DSE::crypto::BoxSession::~BoxSession(){
    if(enc_ctx)     EVP_CIPHER_CTX_free(enc_ctx);
    if(dec_ctx)     EVP_CIPHER_CTX_free(dec_ctx);
}

bool DSE::crypto::BoxSession::init(const unsigned char key[32], const unsigned char iv[12]) noexcept {

    std::memcpy(this->key , key , 32);
    std::memcpy(this->iv , iv , 12);

    if(!(enc_ctx = EVP_CIPHER_CTX_new()))
    {
        DSE_LOG_ERROR(" error in initialising enc ctx ");
        return false;
    }

    if(!(dec_ctx = EVP_CIPHER_CTX_new()))
    {
        DSE_LOG_ERROR(" error in initialising dec ctx ");
        return false;
    }

    if(1 != EVP_EncryptInit_ex(enc_ctx , EVP_aes_256_gcm() , NULL , NULL , NULL)){
        DSE_LOG_ERROR(" error in EVP_EncryptInit_ex for enc ctx ");
        return false;
    }

    if(1 != EVP_DecryptInit_ex(dec_ctx , EVP_aes_256_gcm() , NULL , NULL , NULL)){
        DSE_LOG_ERROR(" error in EVP_DecryptInit_ex for dec ctx ");
        return false;
    }

    if( 1 != EVP_CIPHER_CTX_ctrl(enc_ctx , EVP_CTRL_GCM_SET_IVLEN , IV_LEN , NULL)){
        DSE_LOG_ERROR(" error in setting iv_len for enc ctx ");
        return false;
    }

    if( 1 != EVP_CIPHER_CTX_ctrl(dec_ctx , EVP_CTRL_GCM_SET_IVLEN , IV_LEN , NULL)){
        DSE_LOG_ERROR(" error in setting iv_len for dec ctx ");
        return false;
    }

    return true;
}

void DSE::crypto::BoxSession::build_iv(unsigned char out[12] , uint64_t seq) const noexcept{
    std::memcpy(out , iv, 12);
    for(int i =0; i<8; i++){
        out[11-i]^=static_cast<unsigned char>(seq>>(i*8));
    }
}

bool DSE::crypto::BoxSession::encrypt(const unsigned char* plaintext , size_t plain , const unsigned char* aad , size_t aad_len , unsigned char* ciphertext , unsigned char tag[16]) noexcept {

    unsigned char iv[12];
    build_iv(iv , tx_seq++);

    if( 1 != EVP_EncryptInit_ex(enc_ctx , NULL , NULL , key ,iv)){
        DSE_LOG_ERROR(" error in setting key and iv for enc ctx ");
        return false;
    }
    int outlen = 0;
    if(aad_len && EVP_EncryptUpdate(enc_ctx , NULL , &outlen , aad , static_cast<int>(aad_len)) != 1){
        DSE_LOG_ERROR(" error in EVP_EncryptUpdate function for enc ctx ");
        return false;
    }

    if(1 != EVP_EncryptUpdate(enc_ctx , ciphertext , &outlen , plaintext , static_cast<int>(plain))){
        DSE_LOG_ERROR(" error in plaintext EVP_EncryptUpdate for enc ctx ");
        return false;
    }

    int final_len = 0;
    if(1 != EVP_EncryptFinal_ex(enc_ctx , ciphertext + outlen , &final_len)){
        DSE_LOG_ERROR(" error in EVP_EncryptFinal_ex for enc ctx ");
        return false;
    }

    return EVP_CIPHER_CTX_ctrl(enc_ctx , EVP_CTRL_GCM_GET_TAG , 16 , tag) == 1;
}

bool DSE::crypto::BoxSession::decrypt(const unsigned char* ciphertext, size_t clen , const unsigned char* aad , size_t aad_len , const unsigned char tag[16], unsigned char* plaintext ) noexcept {

    unsigned char iv[12];
    build_iv(iv , rx_seq++);

    if( 1 != EVP_DecryptInit_ex(dec_ctx , NULL , NULL , key , iv)){
        DSE_LOG_ERROR(" error in setting key and iv for enc ctx ");
        return false;
    }
    int outlen = 0;

    if(aad_len && EVP_DecryptUpdate(dec_ctx , NULL , &outlen , aad , static_cast<int>(aad_len)) != 1){
        DSE_LOG_ERROR(" error in EVP_DecryptUpdate function for dec ctx ");
        return false;
    }

    if( 1 != EVP_DecryptUpdate(dec_ctx , plaintext , &outlen , ciphertext , static_cast<int>(clen))){
        DSE_LOG_ERROR(" error in evp_decryptupdate function for dec ctx ");
        return false;
    }

    if( 1 != EVP_CIPHER_CTX_ctrl(dec_ctx, EVP_CTRL_GCM_SET_TAG, 16,const_cast<unsigned char*>(tag))){
        DSE_LOG_ERROR(" error in evp cipher ctx ctrl function for dec ctx ");
        return false;
    }
    
    int final_len = 0;
    return EVP_DecryptFinal_ex(dec_ctx, plaintext + outlen, &final_len) == 1;

}