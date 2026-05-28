#pragma once

#include<openssl/evp.h>
#include<openssl/err.h>
#include<cstdint>

namespace DSE::crypto{
    class BoxSession{
        private:
        EVP_CIPHER_CTX* enc_ctx = nullptr;
        EVP_CIPHER_CTX* dec_ctx = nullptr;

        unsigned char key[32]{};
        unsigned char iv[12]{};
        uint64_t tx_seq = 0;
        uint64_t rx_seq = 0;

        void build_iv(unsigned char out[12] , uint64_t seq) const noexcept;

        public:
        static constexpr size_t KEY_LEN = 32;
        static constexpr size_t IV_LEN = 12;
        static constexpr size_t TAG_LEN = 16;
        
        BoxSession() = default;
        ~BoxSession();
        BoxSession(const BoxSession&)            = delete;
        BoxSession& operator=(const BoxSession&) = delete;

        bool init(const unsigned char key[32], const unsigned char iv[12]) noexcept;
        bool encrypt(const unsigned char* plaintext , size_t plain , const unsigned char* aad , size_t aad_len , unsigned char* ciphertext , unsigned char tag[16]) noexcept;
        bool decrypt(const unsigned char* ciphertext, size_t clen , const unsigned char* aad , size_t aad_len , const unsigned char tag[16], unsigned char* plaintext ) noexcept;
    };
}