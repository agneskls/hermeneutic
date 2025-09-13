

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

class pb_client : public base_client<pb_client>
{
public:
    pb_client(const std::string& connection_str, std::vector<int> bps)
        : base_client<pb_client>(connection_str), bps_(bps) {

            for (auto& i : bps) {
            printf("%d ", i );
        }
        printf(" B | A ");
        for (auto& i : bps) {
            printf("%d ", i );
        }
        printf("\n");
        fflush(stdout);
    }

    void process_ticks(const batched_tick_update &ticks) 
    {
        book_.update_ticks(ticks);

        auto bb = book_.best_bid().price;
        auto ba = book_.best_ask().price;

        if (!is_equal(best_bid_, bb) || !is_equal(best_ask_, ba))
        {

            auto bids = book_.price_band(bb, bps_);
            auto asks = book_.price_band(ba, bps_);

            for (auto& i : bids) {
                printf("%f ", i );
            }
            printf(" | ");
            for (auto& i : asks) {
                printf("%f ", i );
            }
            printf("\n");
            fflush(stdout);
        }

        return;
    }

private:
    extended_book book_;
    double best_bid_;
    double best_ask_;
    std::vector<int> bps_;
};

int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    file_log_sink logger("client3");

    std::string connection_str = absl::GetFlag(FLAGS_target);

    std::vector<int> bps = { 0, 50, 100, 200, 500, 1000};

    pb_client client(connection_str, bps);
    client.subscribe_symbol("BTCUSDT");

    while (true)
    {
        bool got_message = client.poll();
    }

    return 0;
}
