
# Design Architecture
Built on a single server handling multiple clients, with protobuf for compact message encoding and gRPC for streamlined remote procedure calls.

## Aggregator - the server
The server—referred to as the Aggregator, establishes three persistent WebSocket connections to external exchanges: Binance, Kraken, and Crypto.com. It runs a central event loop that continuously polls incoming data streams from these exchanges, while also handling gRPC requests from connected clients. The Aggregator acts as the bridge between real-time market feeds and client-side consumers, it normalizes market data from multiple exchanges. It abstracts away the differences in WebSocket payloads, transforming them into a unified internal representation.

### market protocol
Each of the three exchange integrations—Binance, Kraken, and Crypto.com—is implemented as a separate module under src/market_protocol. These modules encapsulate the logic for handling WebSocket connections and parsing exchange-specific data formats. To streamline common functionality, they all rely on a shared utility called wws_link, which centralizes reusable components like connection helpers, URL builders, or request formatters.

#### API of the exchange:

    * [Binance] (https://developers.binance.com/docs/binance-spot-api-docs/web-socket-streams#individual-symbol-ticker-streams)
    * [Kraken] (https://docs.kraken.com/api/docs/websocket-v2/book/)
    * [crypto.com] (https://exchange-docs.crypto.com/exchange/v1/rest-ws/index.html#book-instrument_name-depth)

#### Notes on heartbeat handling:
| Exchange | Notes |
| -------- | ----- |
| Binance | on heartbeat request from exchange(websocket level), reply with respond(websocket level) |
| Kraken | send ping message to market periodically |
| Crypto.com | on heartbeat request from exchange(message level), reply with respond(message level) |


## Client
The system includes three client implementations, each inheriting from a shared base_client class. This base class encapsulates the gRPC client interface logic and handles the connection lifecycle with the Aggregator server. Upon initialization, each client connects to the Aggregator and subscribes to its data stream.

The Aggregator responds by sending a continuous stream of batched_tick_update messages. These messages are received by base_client, which then dispatches them to the appropriate client-specific handler for further processing—allowing each client to interpret and act on the data according to its own logic.

Each client maintains its own instance of the extended_book class, allowing tick data from Binance, Kraken, and Crypto.com to be processed independently while still conforming to a unified data model. Internally, extended_book uses a std::map keyed by tick price, ensuring that all entries are stored in ascending order by default. This structure makes it efficient to traverse price levels and perform range-based operations.

Despite being fed by separate clients, all extended_book instances share the same logic and structure, enabling consistent handling of market data across exchanges. The class also provides built-in utilities to compute volume bands and price bands, giving each client the ability to analyze liquidity and price distribution in a standardized way.

Each tick in the system carries four distinct quantity fields:
| Field  | Remarks |
| ------ | ------- |
| qty[0] | Aggregated quantity across all exchanges |
| qty[1] | Quantity from Binance |
| qty[2] | Quantity from Kraken |
| qty[3] | Quantity from Crypto.com |

The extended_book class uses this structure to maintain a unified view of market depth. All higher-level calculations—such as Best Bid/Offer (BBO), price bands, and volume bands—are derived from the aggregated quantity (qty[0]), ensuring consistency across analytics.

Because the data originates from multiple exchanges, it's entirely possible for the bid and ask prices to cross—a reflection of fragmented liquidity and asynchronous updates across markets. 

## Test suite
The project uses Google Test (gtest) as its testing framework. All test cases are organized under the src/tests directory.

To execute the test suite, run:
```
ctest
```


# Compilation Instruction
## Docker dev box

Docker files for the dev box could be found in under docker-devbox.
```
cd hermeneutic/docker-devbox
```

* Build the image and start the container
```
docker-compose up --build -d
```

* Enter into interactive shell
```
docker exec -it rocky-cpp-dev bash
```

It is an environmet installed with cmake / gcc / vckpg etc on rocky linux 9.

## Compilation
Login into docker-devbox interactive shell.

1. Clone the repository recursively
```
git clone --recurse-submodules https://hermeneutic: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@github.com/agneskls/hermeneutic.git
```
2. cd into the source directory
```
cd hermeneutic
```
3. run cmake
```
cmake -S . -B build
```
this command takes long to finish. It will try to compile the third party libraries.

4. build the project
```
cd build
make -j 4
```
This will build the server, 3 clients and unit test.




# Run the applications from docker hub directly
* Run the docker container in detached mode
```
docker run -dit --init  --name aggsrv agneskls/agg-service  /bin/bash
```
* Start the server / clients
```
docker exec aggsrv ./bin/server
docker exec aggsrv ./bin/client1
docker exec aggsrv ./bin/client2
docker exec aggsrv ./bin/client3
```

# Consideration / Limitation
## The choice of third party library
### Poco
The project currently uses the POCO library for handling WebSocket connections and JSON parsing. While alternatives like Boost.Asio for networking and nlohmann/json for JSON handling are widely adopted and performant, POCO offers a convenient all-in-one solution that’s well-suited for rapid prototyping. That said, it would be worthwhile to benchmark POCO against other libraries in terms of performance and flexibility—especially if the system scales or moves beyond prototype stage. In the long run, even implementing a custom WebSocket layer could be considered for tighter control and optimization.

### gRPC
gRPC is used as the communication protocol between the Aggregator server and clients, as per project requirements. While gRPC offers strong support for cross-language communication and efficient binary serialization via Protocol Buffers, its performance characteristics in C++—especially under high-throughput, low-latency conditions—are still an area I’m exploring. Given limited prior experience with gRPC, further research and profiling would be beneficial to understand its behavior under load and to fine-tune its integration for production-grade reliability.

## Data Recovery
The current system does not implement any form of data recovery or caching, either between the server and the exchanges, or between the server and its clients. This design relies on the assumption that markets are fast-moving and that the full market book will be reconstructed quickly from live tick streams.

While this may hold true under ideal conditions, it's a risky assumption—especially in cases of network latency, dropped connections. Without recovery mechanisms, clients may experience gaps in market data, leading to inaccurate views of liquidity or pricing.

Addressing this limitation would involve:
Implementing market recovery protocol
Implementing local caching or snapshot recovery strategies
Considering persistent storage for replaying recent market states

This is a critical area for future development.


## Test suite improvement
Currently, the project includes only minimal test coverage, primarily intended as a prototype to demonstrate how the unit test suite is integrated into the build system. The existing test cases serve as examples for setting up and executing tests using Google Test (gtest) and CTest.

This foundation makes it easy for contributors to expand the suite with more comprehensive tests as the codebase matures. It also ensures that the testing infrastructure is already in place for future CI/CD integration and automated validation.


