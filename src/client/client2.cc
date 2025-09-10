

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

class vb_client : public base_client<vb_client>
{
public:
    vb_client(const std::string& connection_str, std::vector<double> bands)
        : base_client<vb_client>(connection_str),
        bands_(bands)
    {
    }

    void process_ticks(const batched_tick_update &ticks)
    {
        book_.update_ticks(ticks);
        auto bids = book_.volume_band_bids(bands_ );
        auto asks = book_.volume_band_asks(bands_ );

        for (auto p : bids) {
            double f = p;
            printf("%f ", f);
        }
        printf(" | ");
        for (auto p : asks) {
            double f = p;
            printf("%f ", f);
        }
        printf("\n");
    }

private:
    extended_book book_;
    std::vector<double> bands_;
};

int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    file_log_sink logger("client2");

    std::string connection_str = absl::GetFlag(FLAGS_target);

    std::vector<double> bands =  { 1'000'000, 5'000'000, 10'000'000, 25'000'000, 50'000'000} ;

    vb_client client(connection_str, bands);
    client.subscribe_symbol("BTCUSDT");

    while (true)
    {
        bool got_message = client.poll();
    }

    return 0;
}
