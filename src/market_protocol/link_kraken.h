#ifndef _LINK_KRAKEN_H_
#define _LINK_KRAKEN_H_   



#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>


#include "wws_link.h"
#include "../common.h"
#include "../logger.h"

#include "../protos/aggregator.grpc.pb.h"

#include <chrono>


using agg_proto::tick_update;
using agg_proto::batched_tick_update;


namespace market_protocol {

class kraken_link {
private:    
    static constexpr exchange_t exchange = exchange_t::kraken;
    const std::string end_point = "wss://ws.kraken.com:443";
    std::function<void(const batched_tick_update&)> callback_;
    wws_link websocket_;
    std::string url_;
    std::unique_ptr<Poco::Net::WebSocket> ws_;
    std::string symbol_;
    std::string market_symbol_;
    std::chrono::steady_clock::time_point last_heartbeat_time_;
    static const int HEARTBEAT_INTERVAL_SECONDS = 10;

    
/*
https://docs.kraken.com/api/docs/websocket-v2/book

{
    "method": "subscribe",
    "params": {
        "channel": "book",
        "symbol": [
            "ALGO/USD",
            "MATIC/USD"
        ]
    }
}
    
*/

public:
    kraken_link(std::string symbol, std::string market_symbol) : 
        symbol_(symbol),
        market_symbol_(market_symbol)
    {
        url_ = end_point + "/v2";
        last_heartbeat_time_ = std::chrono::steady_clock::now();
    }

    void connect() {
        websocket_.connect(url_);


        Poco::JSON::Object::Ptr json = new Poco::JSON::Object;
        json->set("method", "subscribe");
        Poco::JSON::Object::Ptr params = new Poco::JSON::Object;
        params->set("channel", "book");
        Poco::JSON::Array::Ptr symbols = new Poco::JSON::Array;
        symbols->add(market_symbol_);
        params->set("symbol", symbols);
        json->set("params", params);

        std::stringstream ss;
        Poco::JSON::Stringifier::stringify(json, ss);
        std::string jsonStr = ss.str();
     
        websocket_.send_message(jsonStr.c_str(), jsonStr.length());
    }

    void set_callback(std::function<void(const batched_tick_update&)> cb) {
        callback_ = std::move(cb);
    }

    void send_ping() {
        static const std::string ping_msg = R"({"method": "ping", "req_id": 101})";

        websocket_.send_message(ping_msg.c_str(), ping_msg.length());
    }

    bool check_heartbeat() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat_time_).count();
        
        if (elapsed_seconds >= HEARTBEAT_INTERVAL_SECONDS) {
            last_heartbeat_time_ = now;
            send_ping();
            return true;
        }
        return false;
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

                if (obj->has("channel")) {
                    std::string channel = obj->getValue<std::string>("channel");
                    if (channel == "book") {
                        if (obj->has("type") && (obj->getValue<std::string>("type") == "update") )
                            handle_depth(obj);
                    }
                }

                return true;
            } catch (const Poco::Exception& ex) {
                LOG(ERROR) << "POCO Exception: " << ex.displayText();
            } catch (const std::exception& ex) {
                LOG(ERROR) << "Exception: " << ex.what();
            }
        } else 
            check_heartbeat();
        return false;
    }

protected:        
    void handle_depth(Poco::JSON::Object::Ptr obj) {
        batched_tick_update ticks;


        Poco::JSON::Array::Ptr dataArray = obj->getArray("data");
        if (!dataArray->empty()) {
            Poco::JSON::Object::Ptr entry = dataArray->getObject(0);
            ticks.set_exchange((uint32_t) exchange);
            // ticks.set_symbol(entry->getValue<std::string>("symbol"));
            ticks.set_symbol(symbol_);
            //ticks.set_tick_id(obj->getValue<int64_t>("timestamp")); 
            auto update_ticks = [&] (Poco::JSON::Array::Ptr tick_array, side_t side) {
                for (size_t i = 0; i < tick_array->size(); ++i) {
                    auto& tick = *ticks.add_updates();
                    //tick.set_tick_id(obj->getValue<int64_t>("timestamp")); 

                    Poco::JSON::Object::Ptr bid = tick_array->getObject(i);
                    tick.set_price(bid->getValue<double>("price"));
                    tick.set_quantity(bid->getValue<double>("qty"));

                    tick.set_side((uint8_t) side);
                }
            };

            Poco::JSON::Array::Ptr bids = entry->getArray("bids");
            Poco::JSON::Array::Ptr asks = entry->getArray("asks");

            if (callback_) {
                update_ticks(bids, side_t::bid);
                update_ticks(asks, side_t::ask);
                callback_(ticks);
            }

        }


    }


};

}   // namespace market_protocol

#endif  // _LINK_KRAKEN_H_