

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include "../protos/aggregator.grpc.pb.h"

#include "base_client.h"
#include "orderbook.h"
#include "../logger.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using agg_proto::batched_tick_update;
using namespace order_book;

class bbo_client : public base_client<bbo_client>
{
public:
    bbo_client(const std::string& connection_str)
        : base_client<bbo_client>(connection_str) {
        printf("bid_price @ bid_total_qty | ask_price @ ask_total_qty\n");
    }

    void process_ticks(const batched_tick_update &ticks)
    {
        book_.update_ticks(ticks);

        auto bb = book_.best_bid();
        auto ba = book_.best_ask();

        bool print = false;
        int x = 0;
        if (!is_equal(bb.price, best_bid_.price)) {
            print = true;
        }
        else if (!is_equal(bb.qty[0], best_bid_.qty[0])) {
            print = true;
        }
        else if (!is_equal(ba.price, best_ask_.price)) {
            print = true;
        }
        else if (!is_equal(ba.qty[0], best_ask_.qty[0])) {
            print = true;
        }


        if (print) {
            printf("%f @ %f | %f @ %f\n", bb.price, bb.qty[0], ba.price, ba.qty[0]);
        }

        best_bid_ = bb;
        best_ask_ = ba;
    }

private:
    extended_book book_;
    tick_data best_bid_;
    tick_data best_ask_;
};

int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    file_log_sink logger("client1");

    std::string connection_str = absl::GetFlag(FLAGS_target);

    bbo_client client(connection_str);
    client.subscribe_symbol("BTCUSDT");

    while (true)
    {
        bool got_message = client.poll();
    }

    return 0;
}
