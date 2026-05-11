#pragma once

#include<cstdint>

namespace DSE::fo{

#pragma pack(push , 1)
struct WireHeader{
    uint16_t packet_length;
    uint32_t sequence_number;
    char checksum[16];
};
static_assert(sizeof(WireHeader) == 22 , "WireHeader must be 22 bytes");

struct Message_Header{
    std::int16_t TransactionCode;
    std::int32_t LogTime;
    char AlphaChar[2];
    std::int32_t TraderId;
    std::int16_t ErrorCode;
    std::int64_t Timestamp;
    char Timestamp1[8];
    char Timestamp2[8];
    std::int16_t MessageLength;
};
static_assert(sizeof(Message_Header) == 40 , "MESSAGE_HEADER must be 40 bytes");

struct MS_GR_REQUEST{
    Message_Header header;
    int16_t BoxId;
    char BrokerId[5];
    char Filler;
};
static_assert(sizeof(MS_GR_REQUEST) ==48 , "MS_GR_REQUEST must be 48 bytes");


struct MS_GR_RESPONSE{
    Message_Header Header;
    std::int16_t BoxId;
    char BrokerId[5];
    char Filler;
    char IPAddress[16];
    std::int32_t Port;
    char SessionKey[8];
    char CryptographicKey[32];
    char CryptographicIV[16];
};
static_assert(sizeof(MS_GR_RESPONSE) == 124);

#pragma pack(pop)


}