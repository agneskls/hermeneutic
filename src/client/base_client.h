#ifndef __BASE_CLIENT_H__
#define __BASE_CLIENT_H__


#include <grpcpp/grpcpp.h>
#include "../protos/aggregator.grpc.pb.h"
#include "../logger.h"



using agg_proto::agg_service;
using agg_proto::tick_request;
using agg_proto::tick_update;
using agg_proto::batched_tick_update;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

template <typename Derived>
class base_client {
private:
    std::shared_ptr<Channel> channel_;
    std::unique_ptr<agg_service::Stub> stub_;
    std::unique_ptr<grpc::ClientContext> context_;
    std::unique_ptr<grpc::ClientReader<batched_tick_update>> reader_;
public:
    base_client(const std::string& connection_str)
    {
        channel_ = grpc::CreateChannel(connection_str, grpc::InsecureChannelCredentials());
        stub_ = agg_service::NewStub(channel_);
        context_ = std::make_unique<grpc::ClientContext>();
    }

    void subscribe_symbol(const std::string &symbol)
    {
        tick_request request;
        request.set_symbol(symbol);
        reader_ = stub_->TickBatchedStreamRequest(context_.get(), request);
        LOG(INFO) << "Sent:" << request.ShortDebugString();
    }

    //virtual void process_ticks(const batched_tick_update &ticks) = 0;

    void process_ticks(const batched_tick_update &ticks) {
        static_cast<Derived*>(this)->process_ticks(ticks); // Calls the derived class's implementation
    }

    bool poll()
    {
        if (!reader_)
            return false;
        batched_tick_update ticks;
        if (reader_->Read(&ticks))
        {
            LOG(INFO) << "Received:" << ticks.ShortDebugString();
            process_ticks(ticks);
            return true;
        }
        else
        {
            grpc::Status status = reader_->Finish();
            reader_.reset();
            return false;
        }
        return true;
    }



};



#endif // __BASE_CLIENT_H__
