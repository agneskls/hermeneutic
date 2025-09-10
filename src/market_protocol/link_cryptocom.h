#ifndef _LINK_CRYPTOCOM_H_
#define _LINK_CRYPTOCOM_H_   



#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>


#include "wws_link.h"
#include "../common.h"
#include "../logger.h"

#include "../protos/aggregator.grpc.pb.h"

using agg_proto::tick_update;
using agg_proto::batched_tick_update;



namespace market_protocol {

class cryptocom_link {
private:
    static constexpr exchange_t exchange = exchange_t::cryptocom;
    const std::string end_point = "wss://stream.crypto.com:443";
    std::function<void(const batched_tick_update&)> callback_;
    wws_link websocket_;
    std::string url_;
    std::unique_ptr<Poco::Net::WebSocket> ws_;
    std::string symbol_;
    std::string market_symbol_;

    
/*
https://exchange-docs.crypto.com/exchange/v1/rest-ws/index.html#book-instrument_name-depth
{
  "method": "subscribe",
  "params": {
    "channels": ["book.BTC_USDT"]
  },
  "id": 1
}

    
*/

public:
    cryptocom_link(std::string symbol, std::string market_symbol) : 
        symbol_(symbol),
        market_symbol_(market_symbol)
    {
        url_ = end_point + "/exchange/v1/market";
    }

    void connect() {
        websocket_.connect(url_);


        Poco::JSON::Object::Ptr json = new Poco::JSON::Object;
        json->set("id", 1);
        json->set("method", "subscribe");
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object;
        Poco::JSON::Array::Ptr channels = new Poco::JSON::Array;
        channels->add("book." + market_symbol_ + ".50");
        params->set("channels", channels);
        params->set("book_subscription_type", "SNAPSHOT_AND_UPDATE");
        params->set("book_update_frequency", 10);
        json->set("params", params);

        std::stringstream ss;
        Poco::JSON::Stringifier::stringify(json, ss);
        std::string jsonStr = ss.str();
     
        websocket_.send_message(jsonStr.c_str(), jsonStr.length());
    }

    void set_callback(std::function<void(const batched_tick_update&)> cb) {
        callback_ = std::move(cb);
    }

    bool poll() {
        char buffer[200000];
        int n = websocket_.poll(buffer, sizeof(buffer));
        if (n > 0) {
            try {
                std::string msg(buffer, n);
                LOG(INFO) << "Received: " << msg;

                Poco::JSON::Parser parser;
                Poco::Dynamic::Var result = parser.parse(msg);
                Poco::JSON::Object::Ptr obj = result.extract<Poco::JSON::Object::Ptr>();

                std::string method = obj->getValue<std::string>("method");
                if (method == "subscribe") {

                    if (obj->has("result")) {
                        Poco::JSON::Object::Ptr result = obj->getObject("result");
                        if (result->has("data"))
                            handle_depth(result);
                    }
                }
                else if (method == "public/heartbeat") {

                    
                    Poco::JSON::Object::Ptr json = new Poco::JSON::Object;
                    json->set("method", "public/respond-heartbeat");
                    json->set("id", obj->getValue<int64_t>("id"));

                    std::stringstream ss;
                    Poco::JSON::Stringifier::stringify(json, ss);
                    std::string jsonStr = ss.str();
                
                    websocket_.send_message(jsonStr.c_str(), jsonStr.length());
                    
                    
                }
                return true;
            } catch (const Poco::Exception& ex) {
                LOG(ERROR) << "POCO Exception: " << ex.displayText();
            } catch (const std::exception& ex) {
                LOG(ERROR) << "Exception: " << ex.what();
            }
        }
        return false;
    }

protected:        
    void handle_depth(Poco::JSON::Object::Ptr obj) {
        batched_tick_update ticks;

        std::string channel = obj->getValue<std::string>("channel");
        bool is_update = channel == "book.update";
        if (is_update)
            ticks.set_flag((uint64_t) updata_flag_t::underfined);
        else
            ticks.set_flag((uint64_t) updata_flag_t::snapshot);
        

        Poco::JSON::Array::Ptr dataArray = obj->getArray("data");
        if (!dataArray->empty()) {
            Poco::JSON::Object::Ptr entry = dataArray->getObject(0);
            ticks.set_exchange((uint32_t) exchange);
            // ticks.set_symbol(entry->getValue<std::string>("symbol"));
            ticks.set_symbol(symbol_);
            ticks.set_tick_id(entry->getValue<int64_t>("t")); 
            auto update_ticks = [&] (Poco::JSON::Array::Ptr tick_array, side_t side) {
                for (size_t i = 0; i < tick_array->size(); ++i) {
                    auto& tick = *ticks.add_updates();
                    tick.set_tick_id(entry->getValue<int64_t>("t")); 

                    Poco::JSON::Array::Ptr atick = tick_array->getArray(i);
                    tick.set_price(atick->get(0).convert<double>());
                    tick.set_quantity(atick->get(1).convert<double>());
                    tick.set_side((uint8_t) side);
                }
            };

            Poco::JSON::Array::Ptr bids, asks;
            if (is_update) {
                Poco::JSON::Object::Ptr update = entry->getObject("update");
                bids = update->getArray("bids");
                asks = update->getArray("asks");
            } else {
                bids = entry->getArray("bids");
                asks = entry->getArray("asks");
            }

            if (callback_) {
                update_ticks(bids, side_t::bid);
                update_ticks(asks, side_t::ask);
                callback_(ticks);
            }

        }


    }


};

}   // namespace market_protocol

#endif  // _LINK_CRYPTOCOM_H_