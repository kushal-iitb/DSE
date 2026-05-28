#pragma once

#include<cstdint>
#include<map>
#include<vector>
#include<atomic>

#include "logging_object.hpp"
#include "bswap.hpp"
#include "nse_fo_structs.hpp"
#include "spsc_queue.hpp"

namespace DSE::matching_engine{

    struct OrderInfo{
        uint32_t traderId;
        int32_t price;
        uint32_t qyt;
        uint32_t orderId;
        OrderInfo* prev =nullptr , *next = nullptr;
    };

    struct Level{
        OrderInfo* head = nullptr;
        OrderInfo* tail = nullptr;
        uint64_t total_qty =0;
    };

    class matchingEngine{
        private:
        std::map<uint32_t , std::map<uint32_t , Level , std::greater<> >> tokenByOrderIdBid;
        std::map<uint32_t , std::map<uint32_t , Level >> tokenByOrderIdAsk;
        std::map<uint32_t , OrderInfo> OrderIdByPriceQty;
        std::atomic<uint32_t> nextId{1};
        DSE::spsc::SpscQueue* tbt_queue = nullptr;
        uint32_t tbt_seq{0};

        public:
        void onNewOrder(DSE::fo::MS_OE_REQUEST_TR& oe_request);
        void onModifyOrder(DSE::fo::MS_OM_REQUEST_TR& om_request);
        void onCancelOrder(DSE::fo::MS_OM_REQUEST_TR& om_request);
        void onTrade(int32_t TokenNo , uint32_t buyOrderId , uint32_t sellOrderId , int32_t fill_qty);
        std::uint32_t generateNewOrderId();
        void reconcile(int32_t TokenNo , int32_t qty,uint32_t orderId , int16_t BuySellIndicator);
        void set_tbt_queue(DSE::spsc::SpscQueue* tbt_queue){
            this->tbt_queue = tbt_queue;
        }

    };
} //namespace DSE::matching_engine