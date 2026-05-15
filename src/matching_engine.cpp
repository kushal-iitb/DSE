#include "matching_engine.hpp"

#include <cstring>
#include <endian.h>

namespace {
inline uint32_t order_num_to_id(double d) {
    uint64_t raw;
    std::memcpy(&raw, &d, sizeof(raw));
    return static_cast<uint32_t>(be64toh(raw));
}
}

void DSE::matching_engine::matchingEngine::onNewOrder(DSE::fo::MS_OE_REQUEST_TR& oe_request){

        std::int32_t TokenNo = DSE::bswap::bswap32(oe_request.TokenNo);
        std::int32_t price = DSE::bswap::bswap32(oe_request.Price);
        std::int32_t qty = DSE::bswap::bswap32(oe_request.Volume);
        std::int16_t BuySellIndicator = DSE::bswap::bswap16(oe_request.BuySellIndicator);

        std::uint32_t orderId = generateNewOrderId();

        auto[it , _] = OrderIdByPriceQty.emplace(orderId , OrderInfo{
            .traderId = (uint32_t)DSE::bswap::bswap32(oe_request.TraderId),
            .price = price,
            .qyt = (uint32_t)qty,
            .orderId = orderId,
        });

        OrderInfo* node = &it->second;

        if(BuySellIndicator==1){
            if(tokenByOrderIdBid[TokenNo][price].tail){
                node->prev = tokenByOrderIdBid[TokenNo][price].tail;
                tokenByOrderIdBid[TokenNo][price].tail->next = node;
                tokenByOrderIdBid[TokenNo][price].tail = node;
            }
            else{
                tokenByOrderIdBid[TokenNo][price].head = tokenByOrderIdBid[TokenNo][price].tail = node;
            }
            tokenByOrderIdBid[TokenNo][price].total_qty+=qty;
        }
        else{
            if(tokenByOrderIdAsk[TokenNo][price].tail){
                node->prev = tokenByOrderIdAsk[TokenNo][price].tail;
                tokenByOrderIdAsk[TokenNo][price].tail->next = node;
                tokenByOrderIdAsk[TokenNo][price].tail = node;
            }
            else{
                tokenByOrderIdAsk[TokenNo][price].head = tokenByOrderIdAsk[TokenNo][price].tail = node;
            }
            tokenByOrderIdAsk[TokenNo][price].total_qty+=qty;
        }
        OrderIdByPriceQty[orderId] = *node;
        if(tbt_queue){
            DSE::fo::OrderMessage message{};
            message.header.msg_len = sizeof(message);
            message.header.stream_id = 1;
            message.header.seq_no = ++tbt_seq;
            message.msg_type = 'N';
            message.timestamp_ns = 10101;
            message.order_id = (double)orderId;
            message.token = TokenNo;
            message.order_type = 'L';
            message.price = price;
            message.quantity = qty;
            tbt_queue->try_push(&message , sizeof(message));
        }
        reconcile(TokenNo , qty , orderId , BuySellIndicator);
}

void DSE::matching_engine::matchingEngine::onModifyOrder(DSE::fo::MS_OM_REQUEST_TR& om_request){
        std::int32_t TokenNo = DSE::bswap::bswap32(om_request.TokenNo);
        std::int32_t price = DSE::bswap::bswap32(om_request.Price);
        std::int32_t qty = DSE::bswap::bswap32(om_request.Volume);
        std::int16_t BuySellIndicator = DSE::bswap::bswap16(om_request.BuySellIndicator);

        std::uint32_t orderId = order_num_to_id(om_request.OrderNumber);

        std::uint32_t old_id = orderId;
        auto old_it = OrderIdByPriceQty.find(old_id);
        if(old_it == OrderIdByPriceQty.end()) return;
        std::int32_t oldPrice = old_it->second.price;
        std::int32_t oldQty = old_it->second.qyt;

        OrderInfo* old_n = &old_it->second;
        Level& old_lvl = (BuySellIndicator == 1)
                       ? tokenByOrderIdBid[TokenNo][oldPrice]
                       : tokenByOrderIdAsk[TokenNo][oldPrice];

        if(old_n->prev) old_n->prev->next = old_n->next; else old_lvl.head = old_n->next;
        if(old_n->next) old_n->next->prev = old_n->prev; else old_lvl.tail = old_n->prev;

        old_lvl.total_qty -= old_n->qyt;
        bool old_empty = (old_lvl.head == nullptr);
        OrderIdByPriceQty.erase(old_it);
        if(old_empty){
            if(BuySellIndicator == 1) tokenByOrderIdBid[TokenNo].erase(oldPrice);
            else                      tokenByOrderIdAsk[TokenNo].erase(oldPrice);
        }

        orderId = generateNewOrderId();

        auto[it , _] = OrderIdByPriceQty.emplace(orderId , OrderInfo{
            .traderId = (uint32_t)DSE::bswap::bswap32(om_request.TraderId),
            .price = price,
            .qyt = (uint32_t)qty,
            .orderId = orderId,
        });

        OrderInfo* node = &it->second;

        if(BuySellIndicator==1){
            if(tokenByOrderIdBid[TokenNo][price].tail){
                node->prev = tokenByOrderIdBid[TokenNo][price].tail;
                tokenByOrderIdBid[TokenNo][price].tail->next = node;
                tokenByOrderIdBid[TokenNo][price].tail = node;
            }
            else{
                tokenByOrderIdBid[TokenNo][price].head = tokenByOrderIdBid[TokenNo][price].tail = node;
            }
            tokenByOrderIdBid[TokenNo][price].total_qty+=qty;
        }
        else{
            if(tokenByOrderIdAsk[TokenNo][price].tail){
                node->prev = tokenByOrderIdAsk[TokenNo][price].tail;
                tokenByOrderIdAsk[TokenNo][price].tail->next = node;
                tokenByOrderIdAsk[TokenNo][price].tail = node;
            }
            else{
                tokenByOrderIdAsk[TokenNo][price].head = tokenByOrderIdAsk[TokenNo][price].tail = node;
            }
            tokenByOrderIdAsk[TokenNo][price].total_qty+=qty;
        }
        OrderIdByPriceQty[orderId] = *node;
            if(tbt_queue){
            DSE::fo::OrderMessage message{};
            message.header.msg_len = sizeof(message);
            message.header.stream_id = 1;
            message.header.seq_no = ++tbt_seq;
            message.msg_type = 'M';
            message.timestamp_ns = 10101;
            message.order_id = (double)orderId;
            message.token = TokenNo;
            message.order_type = 'L';
            message.price = price;
            message.quantity = qty;
            tbt_queue->try_push(&message , sizeof(message));
        }
        reconcile(TokenNo , qty , orderId , BuySellIndicator);

}

void DSE::matching_engine::matchingEngine::onCancelOrder(DSE::fo::MS_OM_REQUEST_TR& om_request){
        std::int32_t TokenNo = DSE::bswap::bswap32(om_request.TokenNo);
        std::int32_t price = DSE::bswap::bswap32(om_request.Price);
        std::int32_t qty = DSE::bswap::bswap32(om_request.Volume);
        std::int16_t BuySellIndicator = DSE::bswap::bswap16(om_request.BuySellIndicator);

        std::uint32_t orderId = order_num_to_id(om_request.OrderNumber);

        auto it = OrderIdByPriceQty.find(orderId);
        if(it == OrderIdByPriceQty.end()) return;
        std::int32_t oldPrice = it->second.price;
        std::int32_t oldQty = it->second.qyt;

        OrderInfo* n = &it->second;
        Level& lvl = (BuySellIndicator == 1)
                   ? tokenByOrderIdBid[TokenNo][oldPrice]
                   : tokenByOrderIdAsk[TokenNo][oldPrice];

        if(n->prev) n->prev->next = n->next; else lvl.head = n->next;
        if(n->next) n->next->prev = n->prev; else lvl.tail = n->prev;

        lvl.total_qty -= n->qyt;
        bool empty_now = (lvl.head == nullptr);
        OrderIdByPriceQty.erase(it);
        if(empty_now){
            if(BuySellIndicator == 1) tokenByOrderIdBid[TokenNo].erase(oldPrice);
            else                      tokenByOrderIdAsk[TokenNo].erase(oldPrice);
        }
            if(tbt_queue){
            DSE::fo::OrderMessage message{};
            message.header.msg_len = sizeof(message);
            message.header.stream_id = 1;
            message.header.seq_no = ++tbt_seq;
            message.msg_type = 'C';
            message.timestamp_ns = 10101;
            message.order_id = (double)orderId;
            message.token = TokenNo;
            message.order_type = 'L';
            message.price = price;
            message.quantity = qty;
            tbt_queue->try_push(&message , sizeof(message));
        }
}

std::uint32_t DSE::matching_engine::matchingEngine::generateNewOrderId(){
    return nextId.fetch_add(1 , std::memory_order_relaxed);
}

void DSE::matching_engine::matchingEngine::onTrade(int32_t TokenNo , uint32_t buyOrderId , uint32_t sellOrderId ){
            if(tbt_queue){
            DSE::fo::OrderMessage message{};
            message.header.msg_len = sizeof(message);
            message.header.stream_id = 1;
            message.header.seq_no = ++tbt_seq;
            message.msg_type = 'T';
            message.timestamp_ns = 10101;
            message.order_id = (double)buyOrderId;
            message.token = TokenNo;
            message.order_type = 'L';
            message.price = OrderIdByPriceQty[buyOrderId].price;
            message.quantity = OrderIdByPriceQty[buyOrderId].qyt;
            tbt_queue->try_push(&message , sizeof(message));

            message.header.msg_len = sizeof(message);
            message.header.stream_id = 1;
            message.header.seq_no = ++tbt_seq;
            message.msg_type = 'T';
            message.timestamp_ns = 10101;
            message.order_id = (double)sellOrderId;
            message.token = TokenNo;
            message.order_type = 'L';
            message.price = OrderIdByPriceQty[sellOrderId].price;
            message.quantity = OrderIdByPriceQty[sellOrderId].qyt;
            tbt_queue->try_push(&message , sizeof(message));
        }
    return;
}

void DSE::matching_engine::matchingEngine::reconcile(int32_t TokenNo , int32_t qty ,uint32_t orderId , int16_t BuySellIndicator){
    uint32_t buyOrderId;
    uint32_t sellOrderId;

    if(BuySellIndicator ==1 ){
        buyOrderId = orderId;
    }
    else{
        sellOrderId = orderId;
    }

    auto biditr = tokenByOrderIdBid[TokenNo].begin();
    auto askitr = tokenByOrderIdAsk[TokenNo].begin();
    if(BuySellIndicator == 1){
    if(tokenByOrderIdAsk[TokenNo].empty()) return; 
    while(biditr->first >= askitr->first && qty>0){
        while(qty>0){
                int32_t askqty = askitr->second.head->qyt;
                if(qty>=askqty){
                    qty -= askqty;
                    askitr->second.total_qty-=askqty;
                    askitr->second.head->qyt = 0;
                    sellOrderId = askitr->second.head->orderId;
                    askitr->second.head = askitr->second.head->next;
                    onTrade(TokenNo , buyOrderId , sellOrderId);
                    if(askitr->second.head == nullptr){
                        askitr = tokenByOrderIdAsk[TokenNo].erase(askitr);
                        if(askitr == tokenByOrderIdAsk[TokenNo].end())
                        break;
                        continue;
                    }
                }
                else{
                    askitr->second.head->qyt = askqty-qty;
                    askitr->second.total_qty-=qty;
                    qty =0;
                    sellOrderId = askitr->second.head->orderId;
                    onTrade(TokenNo , buyOrderId , sellOrderId);
                }
                if(qty>0)
                askitr++;
            }
        }
    }
    else{
        if(tokenByOrderIdBid[TokenNo].empty()) return; 
        while(askitr->first<=biditr->first){
        while(qty>0){
                int32_t bidqty = biditr->second.head->qyt;
                if(qty>=bidqty){
                    qty -= bidqty;
                    bidqty = 0;
                    sellOrderId = biditr->second.head->orderId;
                    biditr->second.head = biditr->second.head->next;
                    onTrade(TokenNo , buyOrderId , sellOrderId);
                }
                else{
                    bidqty -= qty;
                    qty =0;
                    sellOrderId = biditr->second.head->orderId;
                    onTrade(TokenNo , buyOrderId , sellOrderId);
                }
                if(qty>0)
                biditr++;
            }
        }
    }
}