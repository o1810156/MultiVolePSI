#include "sockets/sockets.h"
#include <iostream>
#include "coproto/coproto.h"
#include "coproto/Socket/AsioSocket.h"
#include <future>
// #include <thread>
#include "Defines.h"
#include <optional>

namespace co = coproto;
using co::task;

namespace ma = macoro;
using ma::eager_task, ma::make_eager, ma::sync_wait, ma::when_all_ready, ma::wrap;

using namespace sockets_pg;

task<> ping(boost::asio::io_context& io_context)
{
    MC_BEGIN(task<>, sender = co::AsioSocket(), &io_context);

    MC_AWAIT_SET(sender, co::AsioAcceptor("localhost:1212", io_context));

    MC_AWAIT(sender.send("Hello, world!"));

    MC_END();
}

task<> pong(boost::asio::io_context& io_context)
{
    MC_BEGIN(task<>, receiver = co::AsioSocket(), content = std::string(), &io_context);

    MC_AWAIT_SET(receiver, co::AsioConnect("localhost:1212", io_context));

    MC_AWAIT(receiver.recvResize(content));

    std::cout << "Received: " << content << std::endl;

    MC_END();
}

int main()
{
    boost::asio::io_context io_context;
    std::optional<boost::asio::io_context::work> w(io_context);
    auto f = std::async([&] { io_context.run(); });

    auto a1 = std::async([&]() {
        sync_wait(ping(io_context));
        std::cout << "ping done" << std::endl;
    });

    sync_wait(pong(io_context));
    std::cout << "pong done" << std::endl;

    w.reset();
    f.get();
    a1.get();

    return 0;
}