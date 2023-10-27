#pragma once

#include <tuple>
#include <vector>
#include <memory>
#include "coproto/Socket/Socket.h"
#include "coproto/Socket/LocalAsyncSock.h"
#include "coproto/Socket/AsioSocket.h"
#include "boost/asio.hpp"
#include "Defines.h"

namespace co = coproto;
using co::Socket, co::task, co::AsioSocket, co::LocalAsyncSocket;

namespace ma = macoro;
using ma::eager_task, ma::make_eager, ma::sync_wait, ma::when_all_ready, ma::wrap;

namespace sockets
{
    std::tuple<
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>,
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>>
    createLASocketsForZS(u64 n);

    std::tuple<
        std::vector<std::shared_ptr<LocalAsyncSocket>>,
        std::vector<std::shared_ptr<LocalAsyncSocket>>>
    createLASocketsForRC(u64 n);

    /*
    task<std::tuple<
        std::vector<std::shared_ptr<AsioSocket>>,
        std::vector<std::shared_ptr<AsioSocket>>>>
    createAsioSocketsForZS(
        u64 nParties, u64 myIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 10000
    );

    task<std::shared_ptr<AsioSocket>> createAsioSocketForRC_child(
        u64 nParties, u64 myIdx, u64 dealerIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 20000
    );

    task<std::vector<std::shared_ptr<AsioSocket>>> createAsioSocketsForRC_dealer(
        u64 nParties, u64 myIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 20000
    );
    */

    class IOCWork
    {
    public:
        explicit IOCWork();
        boost::asio::io_context mIOContext;
        std::optional<boost::asio::io_context::work> mWork;
    };

    /*
    class IOCSetsForClient
    {
    public:
        explicit IOSetsForClient() = delete;
        explicit IOSetsForClient(u64 nParties, u64 skipIdx);

        std::vector<IOCWork> sendIocwsForZS;
        std::vector<IOCWork> recvIocwsForZS;
        std::vector<std::future<void>> sendRunFsForZS;
        std::vector<std::future<void>> recvRunFsForZS;

        IOCWork sendIocwForRC;
        std::future<void> sendRunFForRC;
    }
    */
}