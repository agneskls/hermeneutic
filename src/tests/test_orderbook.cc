#include <gtest/gtest.h>
#include "../client/orderbook.h"
#include "../protos/aggregator.grpc.pb.h"
#include "../common.h"


using agg_proto::batched_tick_update;
using namespace order_book;

TEST(OrderBook, Aggregation_Bid) {
    extended_book book;
    {
        batched_tick_update ticks;
        ticks.set_exchange(1);
        ticks.set_symbol("BTCUSDT");
        ticks.set_tick_id(0);
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(110000);
            tick.set_quantity(2.1);
            tick.set_side((uint8_t) side_t::bid);
        }
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(120000);
            tick.set_quantity(2.3);
            tick.set_side((uint8_t) side_t::bid);
        }
        book.update_ticks(ticks);
    }
    {
        batched_tick_update ticks;
        ticks.set_exchange(2);
        ticks.set_symbol("BTCUSDT");
        ticks.set_tick_id(0);
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(100000);
            tick.set_quantity(2.1);
            tick.set_side((uint8_t) side_t::bid);
        }
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(120000);
            tick.set_quantity(1.1);
            tick.set_side((uint8_t) side_t::bid);
        }
        book.update_ticks(ticks);
    }

    auto bb = book.best_bid();
    EXPECT_EQ(bb.price, 120000);
    EXPECT_EQ(bb.qty[0], 3.4);
    EXPECT_EQ(bb.qty[1], 2.3);
    EXPECT_EQ(bb.qty[2], 1.1);
}


TEST(OrderBook, Aggregation_Ask) {
    extended_book book;
    {
        batched_tick_update ticks;
        ticks.set_exchange(1);
        ticks.set_symbol("BTCUSDT");
        ticks.set_tick_id(0);
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(100000);
            tick.set_quantity(1.2);
            tick.set_side((uint8_t) side_t::ask);
        }
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(110000);
            tick.set_quantity(2.1);
            tick.set_side((uint8_t) side_t::ask);
        }
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(120000);
            tick.set_quantity(2.3);
            tick.set_side((uint8_t) side_t::ask);
        }
        book.update_ticks(ticks);
    }
    {
        batched_tick_update ticks;
        ticks.set_exchange(2);
        ticks.set_symbol("BTCUSDT");
        ticks.set_tick_id(0);
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(100000);
            tick.set_quantity(2.1);
            tick.set_side((uint8_t) side_t::ask);
        }
        {
            auto& tick = *ticks.add_updates();
            tick.set_price(120000);
            tick.set_quantity(1.1);
            tick.set_side((uint8_t) side_t::ask);
        }
        book.update_ticks(ticks);
    }

    auto ba = book.best_ask();
    EXPECT_EQ(ba.price, 100000);
    EXPECT_EQ(ba.qty[0], 3.3);
    EXPECT_EQ(ba.qty[1], 1.2);
    EXPECT_EQ(ba.qty[2], 2.1);

}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
