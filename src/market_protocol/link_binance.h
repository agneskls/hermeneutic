#ifndef _LINK_BINANCE_H_
#define _LINK_BINANCE_H_   



#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>


#include "wws_link.h"
#include "../common.h"
#include "../logger.h"

#include "../protos/aggregator.grpc.pb.h"

using agg_proto::tick_update;
using agg_proto::batched_tick_update;


namespace market_protocol {

class binance_link {
private:    
    static constexpr exchange_t exchange = exchange_t::binance;
    const std::string end_point = "wss://data-stream.binance.vision:9443";
    std::function<void(const batched_tick_update&)> callback_;
    wws_link websocket_;
    std::string url_;
    std::unique_ptr<Poco::Net::WebSocket> ws_;
    std::string symbol_;

    
/*
Diff. Depth Stream
{
  "e": "depthUpdate", // Event type
  "E": 1672515782136, // Event time
  "s": "BNBBTC",      // Symbol
  "U": 157,           // First update ID in event
  "u": 160,           // Final update ID in event
  "b": [              // Bids to be updated
    [
      "0.0024",       // Price level to be updated
      "10"            // Quantity
    ]
  ],
  "a": [              // Asks to be updated
    [
      "0.0026",       // Price level to be updated
      "100"           // Quantity
    ]
  ]
}
*/

public:
    binance_link(std::string symbol, std::string market_symbol) : 
        symbol_(symbol)
    {
        url_ = end_point + "/ws/" + market_symbol + "@depth";
    }

    void connect() {
        websocket_.connect(url_);
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

                // std::string event = obj->getValue<std::string>("e");
                handle_depth(obj);

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
        // tick_update tick;
        batched_tick_update ticks;
        ticks.set_exchange((uint32_t) exchange);
        // ticks.set_symbol(obj->getValue<std::string>("s"));
        ticks.set_symbol(symbol_);
        ticks.set_tick_id(obj->getValue<int64_t>("E")); 
        auto update_ticks = [&] (Poco::JSON::Array::Ptr tick_array, side_t side) {
            for (size_t i = 0; i < tick_array->size(); ++i) {
                auto& tick = *ticks.add_updates();
                tick.set_tick_id(obj->getValue<int64_t>("E")); 
                Poco::JSON::Array::Ptr atick = tick_array->getArray(i);
                tick.set_price(atick->getElement<double>(0));
                tick.set_quantity(atick->getElement<double>(1));
                tick.set_side((uint8_t) side);
            }
        };

        Poco::JSON::Array::Ptr bids = obj->getArray("b");
        Poco::JSON::Array::Ptr asks = obj->getArray("a");

        if (callback_) {
            update_ticks(bids, side_t::bid);
            update_ticks(asks, side_t::ask);
            callback_(ticks);
        }
    }


};

}   // namespace market_protocol

#endif  // _LINK_BINANCE_H_