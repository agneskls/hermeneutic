
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/initialize.h"
#include "absl/log/globals.h"
#include "absl/strings/str_format.h"


#include "poco_init.h"
#include "../market_protocol/link_binance.h"
#include "../market_protocol/link_kraken.h"
#include "../market_protocol/link_cryptocom.h"
#include "aggregator_server.h"

#include "../logger.h"

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

using namespace market_protocol;

int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();
    file_log_sink logger("server");

    PocoInit pocoinit("server-poco"); // poco for web socket. 
    
    aggregator_server server(absl::GetFlag(FLAGS_port));    // grpc service

    // setup the web sockets to exchange
    binance_link binance("BTCUSDT", "btcusdt");
    kraken_link kraken("BTCUSDT", "BTC/USDT");
    cryptocom_link crytocom("BTCUSDT", "BTC_USDT");

    binance.set_callback(server.get_process_ticks());
    kraken.set_callback(server.get_process_ticks());
    crytocom.set_callback(server.get_process_ticks());

    binance.connect();
    kraken.connect();
    crytocom.connect();

    while (true)
    {
        server.poll_non_block();
        binance.poll();
        kraken.poll();
        crytocom.poll();
    }

    return 0;
}
