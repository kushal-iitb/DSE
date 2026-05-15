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


struct SECURE_BOX_REGISTRATION_REQUEST{
    Message_Header Header;
    std::int16_t BoxId;
};
static_assert(sizeof(SECURE_BOX_REGISTRATION_REQUEST) == 42);

struct SECURE_BOX_REGISTRATION_RESPONSE{
    Message_Header Header;
};
static_assert(sizeof(SECURE_BOX_REGISTRATION_RESPONSE)==40);

struct BOX_SIGN_ON_REQUEST{
    Message_Header Header;
    std::int16_t BoxId;
    char BrokerId[5];
    char Reserved[5];
    char SessionKey[8];
};
static_assert(sizeof(BOX_SIGN_ON_REQUEST) == 60);

struct BOX_SIGN_ON_RESPONSE{
    Message_Header Header;
    std::int16_t BoxId;
    char Reserved[10];
};
static_assert(sizeof(BOX_SIGN_ON_RESPONSE) == 52);

struct ST_BROKER_ELIGIBILITY_PER_MKT{
    std::int16_t flag;
};
static_assert(sizeof(ST_BROKER_ELIGIBILITY_PER_MKT) == 2);

struct MS_SIGNON{
    Message_Header Header;
    std::int32_t UserID;
    char Reserved1[8];
    char Password[8];
    char Reserved2[8];
    char NewPassword[8];
    char TraderName[26];
    std::int32_t LastPasswordChangeDate;
    char BrokerID[5];
    char Reserved3;
    std::int16_t BranchID;
    std::int32_t VersionNumber;
    std::int32_t Batch2StartTime;
    char HostSwitchContext;
    char Colour[50];
    char Reserved4;
    std::int16_t UserType;
    std::int64_t SequenceNumber;
    char WsClassName[14];
    char BrokerStatus;
    char ShowIndex;
    ST_BROKER_ELIGIBILITY_PER_MKT st_broker_eligibility_per_mkt;
    std::int16_t MemberType;
    char ClearingStatus;
    char BrokerName[25];
    char Reserved5[16];
    char Reserved6[16];
    char Reserved7[16];
};
static_assert(sizeof(MS_SIGNON) == 278);

struct CONTRACT_DESC_TR{
    char InstrumentName[6];
    char Symbol[10];
    std::int32_t ExpiryDate;
    std::int32_t StrikePrice;
    char OptionType[2];
};
static_assert(sizeof(CONTRACT_DESC_TR) == 26);

struct ST_ORDER_FLAGS{
    std::uint16_t flags;
};
static_assert(sizeof(ST_ORDER_FLAGS) == 2);

struct ADDITIONAL_ORDER_FLAGS{
    std::uint8_t flags;
};
static_assert(sizeof(ADDITIONAL_ORDER_FLAGS) == 1);

struct MS_OE_REQUEST_TR{
    std::int16_t TransactionCode;
    std::int32_t UserID;
    std::int16_t ReasonCode;
    std::int32_t TokenNo;
    CONTRACT_DESC_TR ContractDesc;
    char AccountNumber[10];
    std::int16_t BookType;
    std::int16_t BuySellIndicator;
    std::int32_t DisclosedVolume;
    std::int32_t Volume;
    std::int32_t Price;
    std::int32_t GoodTillDate;
    ST_ORDER_FLAGS OrderFlags;
    std::int16_t BranchId;
    std::int32_t TraderId;
    char BrokerId[5];
    char OpenClose;
    char Settlor[12];
    std::int16_t ProClientIndicator;
    ADDITIONAL_ORDER_FLAGS AdditionalOrderFlags;
    char Pad;
    std::int32_t Filler;
    double NnfField;
    char PAN[10];
    std::int32_t AlgoID;
    std::int16_t Reserved1;
    char Reserved2[32];
};
static_assert(sizeof(MS_OE_REQUEST_TR) == 158);

struct MS_OM_REQUEST_TR{
    std::int16_t TransactionCode;
    std::int32_t UserID;
    char ModifiedCancelledBy;
    char Pad1;
    std::int32_t TokenNo;
    CONTRACT_DESC_TR ContractDesc;
    double OrderNumber;
    char AccountNumber[10];
    std::int16_t BookType;
    std::int16_t BuySellIndicator;
    std::int32_t DisclosedVolume;
    std::int32_t DisclosedVolumeRemaining;
    std::int32_t TotalVolumeRemaining;
    std::int32_t Volume;
    std::int32_t VolumeFilledToday;
    std::int32_t Price;
    std::int32_t GoodTillDate;
    std::int32_t EntryDateTime;
    std::int32_t LastModified;
    ST_ORDER_FLAGS OrderFlags;
    std::int16_t BranchId;
    std::int32_t TraderId;
    char BrokerId[5];
    char OpenClose;
    char Settlor[12];
    std::int16_t ProClientIndicator;
    ADDITIONAL_ORDER_FLAGS AdditionalOrderFlags;
    char Pad2;
    char Filler[4];
    double NnfField;
    char PAN[10];
    std::int32_t AlgoID;
    std::int16_t Reserved1;
    std::int64_t LastActivityReference;
    char Reserved2[24];
};
static_assert(sizeof(MS_OM_REQUEST_TR) == 186);


struct TBT_Header{
    uint16_t msg_len;
    uint16_t stream_id;
    uint32_t seq_no;
};
static_assert(sizeof(TBT_Header) == 8);


struct OrderMessage {
	TBT_Header header;
	char msg_type;
	std::int64_t timestamp_ns;
	double order_id;
	std::int32_t token;
	char order_type;
	std::int32_t price;
	std::int32_t quantity;
};

struct TradeMessage {
	TBT_Header header;
	char msg_type;
	std::int64_t timestamp_ns;
	double buy_order_id;
	double sell_order_id;
	std::int32_t token;
	std::int32_t trade_price;
	std::int32_t trade_quantity;
};

#pragma pack(pop)


}