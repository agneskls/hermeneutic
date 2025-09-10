#ifndef _BBO_ORDERBOOK_H_
#define _BBO_ORDERBOOK_H_

#include <map>
#include <unordered_map>
#include <stdint.h>

#include "../common.h"
#include "../protos/aggregator.grpc.pb.h"

using agg_proto::tick_update;
using agg_proto::batched_tick_update;


namespace order_book {


    constexpr int fixed_price_scale = 1'000'000; // 4 d.p.
    #define to_fixed_price(d) (static_cast<int64_t>(d * fixed_price_scale));
    #define fr_fixed_price(f) (static_cast<double>(f) / fixed_price_scale);
    constexpr double price_max = std::numeric_limits<double>::max();
    constexpr double price_min = std::numeric_limits<double>::min();


struct tick_data {
    double price;
    double qty[(uint32_t) exchange_t::total];
    double notional;
};


struct sided_book {

    std::map<int64_t, tick_data> book_;
    // key : price. 
    // value.qty[0]: total qty
    // value.qty[1]: qty in binance
    // value.qty[2]: qty in kraken
    // value.qty[3]: qty in cryptocom
    // all stored as fixed point

    void update_tick(uint32_t exchange, const tick_update& tick) {
        if (exchange < (uint32_t) exchange_t::total) {
            int64_t key = to_fixed_price(tick.price());
            // int64_t new_qty = to_fixed_qty(tick.quantity());
            if (auto it = book_.find(key); it != book_.end()) {
                // to update
                it->second.qty[0] = it->second.qty[0] - it->second.qty[exchange] + tick.quantity();
                it->second.notional = tick.price() * it->second.qty[0];
                if (is_equal(it->second.qty[0], 0)) {
                    book_.erase(it);
                } else {
                    it->second.qty[exchange] = tick.quantity();
                }
            } else {
                if (is_greater(tick.quantity(), 0)) {
                    // to insert
                    auto it2 = book_.insert({
                        key, {}
                    });
                    it2.first->second.price = tick.price();
                    it2.first->second.qty[0] = tick.quantity();
                    it2.first->second.qty[exchange] = tick.quantity();
                    it2.first->second.notional = tick.price() * tick.quantity();
                } else {
                    // new tick with zero qty, ignore
                }
            }
        } else {
            // TODO:: exception handling
        }
    }

    // clear all levels on one exchange
    void clear_book(uint32_t exchange) {
        if (exchange < (uint32_t) exchange_t::total) {
            for (auto it = book_.begin(); it!= book_.end();) {
                it->second.qty[0] -= it->second.qty[exchange];
                it->second.notional = it->second.price * it->second.qty[0];
                if (is_equal(it->second.qty[0], 0)) {
                    book_.erase(it);
                } else {
                    it->second.qty[exchange] = 0;
                    ++it;
                }
            }
        } else {
            // TODO:: exception handling
        }

    }

    tick_data get_highest() const {
        if (!book_.empty()) {
            return book_.rbegin()->second;
        }
        else {
            return {};
        }
    }
    tick_data get_lowest() const {
        if (!book_.empty()) {
            return book_.begin()->second;
        }
        else {
            return {};
        }
    }    
};

// An order for 1 symbol
class basic_book {
    protected:
    sided_book bids_;
    sided_book asks_;

    public:
    void update_ticks(const batched_tick_update& ticks) {
        uint32_t exchange = ticks.exchange();
        uint64_t flag = ticks.flag();
        if (flag == (uint64_t) updata_flag_t::snapshot) {
            bids_.clear_book(exchange);
            asks_.clear_book(exchange);
        }
        for (const auto& tick: ticks.updates()) {
            if (tick.side() == (int) side_t::bid) {
                bids_.update_tick(exchange, tick);
            }
            else if (tick.side() == (int) side_t::ask) {
                asks_.update_tick(exchange, tick);
            }        
        }
    }


};

class extended_book : public basic_book {
    public:

    tick_data best_bid() const {
        if (!bids_.book_.empty()) {
            return bids_.book_.rbegin()->second;
        }
        else {
            return {};
        }
    }
    tick_data best_ask() const {
        if (!asks_.book_.empty()) {
            return asks_.book_.begin()->second;
        }
        else {
            return {};
        }
    }

    template <typename Iterator>
    std::vector<double>  volume_band(Iterator begin, Iterator end, const std::vector<double>& bands) const {
        std::vector<double> prices(bands.size());
        double accum = 0;
        int index = 0;
        for (Iterator it = begin; it!= end; ++it) {
            auto band = bands[index];
            accum += it->second.notional;
            while ((index < bands.size()) && (accum >= band)) {
                prices[index] = it->second.price;
                ++index;
                band = bands[index];
            }
        }
        return prices;
    }
    std::vector<double> volume_band_asks(const std::vector<double>& bands) const {
        return volume_band(asks_.book_.begin(), asks_.book_.end(), bands);
    }

    std::vector<double> volume_band_bids(const std::vector<double>& bands) const {
        return volume_band(bids_.book_.rbegin(), bids_.book_.rend(), bands);
    }

    static std::vector<double> price_band(double price, const std::vector<int>& bps) {
        std::vector<double> result(bps.size());
        std::transform(bps.begin(), bps.end(), result.begin(), [=](int val) {
            return price + price * val / 10000;
        });
        return result;
    }

};

}



#endif // _BBO_ORDERBOOK_H_