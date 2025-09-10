#ifndef _WWS_LINK_H_
#define _WWS_LINK_H_   


#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/URI.h>
#include <Poco/Base64Encoder.h>
#include <sstream>
#include <random>
#include "../logger.h"


namespace market_protocol {



class wws_link {
private:
    std::unique_ptr<Poco::Net::WebSocket> ws_;
    std::string web_socket_key_;

    std::string generateWebSocketKey() {
        std::array<unsigned char, 16> randomBytes;
        std::random_device rd;
        std::generate(randomBytes.begin(), randomBytes.end(), [&]() { return rd(); });

        std::ostringstream oss;
        Poco::Base64Encoder encoder(oss);
        encoder.write(reinterpret_cast<const char*>(randomBytes.data()), randomBytes.size());
        encoder.close();

        return oss.str();
    }

public:
    wws_link() {
        web_socket_key_ = generateWebSocketKey();
    }

    void connect(const std::string &url) {
        // send_message(url);

        Poco::URI uri(url);

        std::string host = uri.getHost();
        unsigned short port = uri.getPort();
        std::string path = uri.getPathEtc();

        Poco::Net::HTTPSClientSession session(host, port);
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
        request.set("Upgrade", "websocket");
        request.set("Connection", "Upgrade");
        request.set("Sec-WebSocket-Key", generateWebSocketKey());
        request.set("Sec-WebSocket-Version", "13");

        LOG(INFO) << "Sending request to " << uri.toString();
        LOG(INFO) << "Request method: " << request.getMethod();
        LOG(INFO) << "Request URI: " << request.getURI();

        Poco::Net::HTTPResponse response;
        LOG(INFO) << "Received response: "<< std::to_string(response.getStatus());
        LOG(INFO) << "Response reason: " << response.getReason();

        try {
            ws_.reset(new Poco::Net::WebSocket(session, request, response));
            ws_->setBlocking(false);
            ws_->setReceiveTimeout(Poco::Timespan(0, 0));
        } catch (const Poco::Exception& ex) {
            LOG(ERROR) << "POCO Exception: " << ex.displayText();
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Exception: " << ex.what();
        }
    }

    int send_message(const char* buffer, int buffer_len) {
        LOG(INFO) << "send_message: " << web_socket_key_ << ": " << std::string(buffer, buffer_len);
        return ws_->sendFrame(buffer, buffer_len, Poco::Net::WebSocket::FRAME_TEXT);
    }

    void send_pong(const char* buffer, int buffer_len) {
        ws_->sendFrame(buffer, buffer_len, Poco::Net::WebSocket::FRAME_OP_PONG);
    }

    // return len polled
    int poll(char* buffer, int buffer_len) {
        int n = 0;
        try {
            int flags;
            if (ws_->poll(Poco::Timespan(0, 1), Poco::Net::Socket::SELECT_READ)) {
                n = ws_->receiveFrame(buffer, buffer_len, flags);

                if ((n > 0) && ((flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) == Poco::Net::WebSocket::FRAME_OP_PING)) {
                    send_pong(buffer, n);
                    return 0;
                }
            }
        } catch (const Poco::TimeoutException& ex) {
        } catch (const Poco::Exception& ex) {
            LOG(ERROR) << "POCO Exception: " << ex.displayText();
        } catch (const std::exception& ex) {
            LOG(ERROR) << "Exception: " << ex.what();
        }
        return n;
    }

};


}   // namespace market_protocol

#endif  // _WWS_LINK_H_


