#ifndef _AGG_SERVER_HPP_
#define _AGG_SERVER_HPP_

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <queue>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "../protos/aggregator.grpc.pb.h"
#include "../logger.h"

using agg_proto::agg_service;
using agg_proto::tick_request;
using agg_proto::tick_update;
using agg_proto::batched_tick_update;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::Status;


class rpc_handler_base
{
public:
    virtual void proceed(bool ok) = 0;
    virtual ~rpc_handler_base() = default;
};

class tick_request_handler : public rpc_handler_base
{
public:
    tick_request_handler(agg_service::AsyncService *service, grpc::ServerCompletionQueue *cq, std::vector<tick_request_handler*>& clients)
        : service_(service), cq_(cq), writer_(&ctx_), status_(call_status::CREATE), writing_(false), clients_(clients)
    {
        proceed(true);
    }

    void proceed(bool ok) override
    {
        if (status_ == call_status::CREATE)
        {
            status_ = call_status::CONNECTED;
            service_->RequestTickBatchedStreamRequest(&ctx_, &request_, &writer_, cq_, cq_, this);
        }
        else if (status_ == call_status::CONNECTED)
        {
            LOG(INFO) << "client connected";
            new tick_request_handler(service_, cq_, clients_); // Spawn next client handler
            clients_.push_back(this);
            status_ = call_status::IDLE;
        }
        else if (status_ == call_status::WRITING) {
            if (ok) {
                mq_.pop();
                writing_ = false;
                try_write();
            }
        }

        else if (status_ == call_status::FINISH)
        {
            remove_client();
            delete this;
        }
    }


    void finish() {
        status_ = call_status::FINISH;
        writer_.Finish(grpc::Status::OK, this);
    }

    void send_update(const batched_tick_update& update) {
        mq_.push(update);
        try_write();
    }

private:

    void remove_client() {
        auto it = std::find(clients_.begin(), clients_.end(), this);
            if (it != clients_.end()) clients_.erase(it);        
    }
    void try_write() {
        if (!writing_ && !mq_.empty()) {
            writing_ = true;
            status_ = call_status::WRITING;

            writer_.Write(mq_.front(), this);
        }
        if (mq_.empty())
            status_ = call_status::IDLE;
    }


    agg_service::AsyncService *service_;
    grpc::ServerCompletionQueue *cq_;
    grpc::ServerContext ctx_;
    tick_request request_;
    grpc::ServerAsyncWriter<batched_tick_update> writer_;
    enum class call_status
    {
        CREATE,
        CONNECTED,
        IDLE,
        WRITING,
        FINISH
    };
    call_status status_;
    std::vector<tick_request_handler*>& clients_;
    std::queue<batched_tick_update> mq_;
    bool writing_;
};




class aggregator_server
{
public:
    aggregator_server(uint16_t port)
    {
        std::string server_address = absl::StrFormat("0.0.0.0:%d", port);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        builder_.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder_.RegisterService(&service_);
        cq_ = builder_.AddCompletionQueue();

        server_ = builder_.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;
        new tick_request_handler(&service_, cq_.get(), clients_);
    }

    // Poll for gRPC events
    void poll_block()
    {
        void *tag;
        bool ok;
        if (cq_->Next(&tag, &ok))
        {
            static_cast<rpc_handler_base *>(tag)->proceed(ok);
        }
    }

    void poll_non_block()
    {
        void *tag;
        bool ok;
        auto now = std::chrono::system_clock::now();
        auto status = cq_->AsyncNext(&tag, &ok, now);
        if (status == grpc::CompletionQueue::GOT_EVENT)
        {
            static_cast<rpc_handler_base *>(tag)->proceed(ok);
        }
    }


    void process_tick(const batched_tick_update& ticks) {
        LOG(INFO) << "To client:" << ticks.ShortDebugString();
        for (auto* client : clients_) {
            client->send_update(ticks);
        }
    }
    
    std::function<void(const batched_tick_update&)> get_process_ticks() {
        return [this] (const batched_tick_update& ticks) {
            return this->process_tick(ticks);
        };
    }




private:


    agg_service::AsyncService service_;
    grpc::ServerBuilder builder_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    std::unique_ptr<grpc::Server> server_;
    std::vector<tick_request_handler*> clients_;
};


#endif