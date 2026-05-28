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
            DSE::fo::OrderMessage cancel_msg{};
            cancel_msg.header.msg_len   = sizeof(cancel_msg);
            cancel_msg.header.stream_id = 1;
            cancel_msg.header.seq_no    = ++tbt_seq;
            cancel_msg.msg_type         = 'C';
            cancel_msg.timestamp_ns     = 10101;
            cancel_msg.order_id         = (double)old_id;
            cancel_msg.token            = TokenNo;
            cancel_msg.order_type       = 'L';
            cancel_msg.price            = oldPrice;
            cancel_msg.quantity         = oldQty;
            tbt_queue->try_push(&cancel_msg, sizeof(cancel_msg));

            DSE::fo::OrderMessage new_msg{};
            new_msg.header.msg_len   = sizeof(new_msg);
            new_msg.header.stream_id = 1;
            new_msg.header.seq_no    = ++tbt_seq;
            new_msg.msg_type         = 'N';
            new_msg.timestamp_ns     = 10101;
            new_msg.order_id         = (double)orderId;
            new_msg.token            = TokenNo;
            new_msg.order_type       = 'L';
            new_msg.price            = price;
            new_msg.quantity         = qty;
            tbt_queue->try_push(&new_msg, sizeof(new_msg));
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

void DSE::matching_engine::matchingEngine::onTrade(int32_t TokenNo , uint32_t buyOrderId , uint32_t sellOrderId , int32_t fill_qty){
    DSE_LOG_INFO(" TRADE  token={}  buyId={}  sellId={}  fill_qty={}", TokenNo, buyOrderId, sellOrderId, fill_qty);
    if(!tbt_queue) return;

    auto buy_it = OrderIdByPriceQty.find(buyOrderId);
    int32_t trade_price = (buy_it != OrderIdByPriceQty.end())
                          ? buy_it->second.price
                          : 0;

    DSE::fo::OrderMessage message{};
    message.header.msg_len   = sizeof(message);
    message.header.stream_id = 1;
    message.header.seq_no    = ++tbt_seq;
    message.msg_type         = 'T';
    message.timestamp_ns     = 10101;
    message.order_id         = (double)buyOrderId;
    message.token            = TokenNo;
    message.order_type       = 'L';
    message.price            = trade_price;
    message.quantity         = fill_qty;
    tbt_queue->try_push(&message, sizeof(message));

    message.header.seq_no = ++tbt_seq;
    message.order_id      = (double)sellOrderId;
    tbt_queue->try_push(&message, sizeof(message));
}

void DSE::matching_engine::matchingEngine::reconcile(int32_t TokenNo , int32_t qty , uint32_t orderId , int16_t BuySellIndicator){
    auto self_it = OrderIdByPriceQty.find(orderId);
    if(self_it == OrderIdByPriceQty.end()) return;
    int32_t order_price = self_it->second.price;
    int32_t orig_qty = qty;

    if(BuySellIndicator == 1){
        auto& asks = tokenByOrderIdAsk[TokenNo];
        while(qty > 0 && !asks.empty()){
            auto askitr = asks.begin();
            if(askitr->first > order_price) break;
            OrderInfo* head = askitr->second.head;
            if(!head){ asks.erase(askitr); continue; }

            int32_t fill;
            if(qty >= (int32_t)head->qyt){
                fill = head->qyt;
                qty -= fill;
                askitr->second.total_qty -= fill;
                uint32_t sellOrderId = head->orderId;
                askitr->second.head = head->next;
                if(askitr->second.head) askitr->second.head->prev = nullptr;
                else                    askitr->second.tail = nullptr;
                OrderIdByPriceQty.erase(sellOrderId);
                if(askitr->second.head == nullptr) asks.erase(askitr);
                onTrade(TokenNo, orderId, sellOrderId, fill);
            } else {
                fill = qty;
                head->qyt -= qty;
                askitr->second.total_qty -= qty;
                qty = 0;
                onTrade(TokenNo, orderId, head->orderId, fill);
            }
        }
    }
    else {
        auto& bids = tokenByOrderIdBid[TokenNo];
        while(qty > 0 && !bids.empty()){
            auto biditr = bids.begin();
            if(biditr->first < order_price) break;
            OrderInfo* head = biditr->second.head;
            if(!head){ bids.erase(biditr); continue; }

            int32_t fill;
            if(qty >= (int32_t)head->qyt){
                fill = head->qyt;
                qty -= fill;
                biditr->second.total_qty -= fill;
                uint32_t buyOrderId = head->orderId;
                biditr->second.head = head->next;
                if(biditr->second.head) biditr->second.head->prev = nullptr;
                else                    biditr->second.tail = nullptr;
                OrderIdByPriceQty.erase(buyOrderId);
                if(biditr->second.head == nullptr) bids.erase(biditr);
                onTrade(TokenNo, buyOrderId, orderId, fill);
            } else {
                fill = qty;
                head->qyt -= qty;
                biditr->second.total_qty -= qty;
                qty = 0;
                onTrade(TokenNo, head->orderId, orderId, fill);
            }
        }
    }

    int32_t matched = orig_qty - qty;
    if(matched <= 0) return;

    auto own_it = OrderIdByPriceQty.find(orderId);
    if(own_it == OrderIdByPriceQty.end()) return;
    OrderInfo* n = &own_it->second;
    Level& lvl = (BuySellIndicator == 1)
               ? tokenByOrderIdBid[TokenNo][order_price]
               : tokenByOrderIdAsk[TokenNo][order_price];

    if(qty == 0){
        if(n->prev) n->prev->next = n->next; else lvl.head = n->next;
        if(n->next) n->next->prev = n->prev; else lvl.tail = n->prev;
        lvl.total_qty -= n->qyt;
        bool empty = (lvl.head == nullptr);
        OrderIdByPriceQty.erase(own_it);
        if(empty){
            if(BuySellIndicator == 1) tokenByOrderIdBid[TokenNo].erase(order_price);
            else                      tokenByOrderIdAsk[TokenNo].erase(order_price);
        }
    } else {
        n->qyt = qty;
        lvl.total_qty -= matched;
    }
}