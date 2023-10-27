#include <iostream>
#include "coproto/coproto.h"
#include "coproto/Socket/Socket.h"
#include "coproto/Socket/BufferingSocket.h"
#include <thread>
// #include "voleOpprf.h"
#include "volePSI/RsOpprf.h"
#include "volePSI/RsPsi.h"

#include "messagePassingExample.h"

namespace co = coproto;
using co::task, co::Socket, co::sync_wait, co::when_all_ready;
// using co::LocalAsyncSocket;
using co::BufferingSocket;

namespace oc = osuCrypto;
using oc::u64, oc::u32, oc::u8, oc::u8,
    oc::block, oc::span, oc::PRNG;

template <typename T>
std::vector<block> toBlockVec(std::vector<T> vec)
{
    std::vector<block> ret(vec.size());
    for (u64 i = 0; i < vec.size(); ++i)
    {
        ret[i] = block(0, vec[i]);
    }
    return ret;
}

template <typename T>
oc::Matrix<T> toMatrix(std::vector<T> vec)
{
    oc::Matrix<T> ret(vec.size(), 1);
    for (u64 i = 0; i < vec.size(); ++i)
    {
        ret(i, 0) = vec[i];
    }

    for (u64 i = 0; i < vec.size(); ++i)
    {
        std::cout << "beep: " << (int)ret(i, 0) << std::endl;
    }

    return ret;
}

const u64 numThreads = 1;

task<> sender_run(
    BufferingSocket &senderSocket,
    u64 setSize,
    std::vector<block> sendSet,
    oc::Matrix<u8> sendProgramedVals)
{
    MC_BEGIN(
        task<>,
        &senderSocket,
        setSize, sendSet, sendProgramedVals,
        sprng = PRNG(block(0, 1000)),
        sender = std::make_unique<volePSI::RsOpprfSender>());

    MC_AWAIT(senderSocket.send("beep"));

    MC_AWAIT(sender->send(setSize, sendSet, sendProgramedVals, sprng, numThreads, senderSocket));

    for (auto b = sendSet.begin(); b != sendSet.end(); b++)
    {
        try
        {
            std::cout << "[s] f(" << *b << ") = " << (int)sender->eval<u8>(*b) << std::endl;
        }
        catch (std::exception &e)
        {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    MC_END();
}

task<> receiver_run(
    BufferingSocket &receiverSocket,
    u64 setSize,
    std::vector<block> recvSet)
{
    MC_BEGIN(
        task<>,
        receiverSocket,
        setSize, recvSet,
        rprng = PRNG(block(0, 2000)),
        receiver = std::make_unique<volePSI::RsOpprfReceiver>(),
        outputs = std::move(oc::Matrix<u8>{}),
        message = std::string());

    MC_AWAIT(receiverSocket.recvResize(message));
    std::cout << "message = " << message << std::endl;

    outputs.resize(setSize, 1);

    // MC_AWAIT(receiver->receive(setSize, recvSet, outputs, rprng, numThreads, receiverSocket));

    MC_AWAIT(receiver->receive(setSize, recvSet, outputs, rprng, numThreads, receiverSocket));

    std::cout << "$$$ received" << std::endl;

    for (u64 i = 0; i < setSize; i++)
    {
        std::cout << "[r] f(" << recvSet[i] << ") = " << (int)outputs(i, 0) << std::endl;
    }

    MC_END();
}

int run(int argc, char **argv)
{
    /*
    auto sockets = LocalAsyncSocket::makePair();
    auto senderSocket = sockets[0];
    auto receiverSocket = sockets[1];
    */
    BufferingSocket senderSocket, receiverSocket;

    u64 setSize = 8;

    std::vector<u64> sendSet_u64 = {1, 2, 3, 4, 5, 6, 7, 8};
    auto sendSet = toBlockVec(sendSet_u64);

    std::vector<u8> sendProgramedVals_u8 = {8, 7, 6, 5, 4, 3, 2, 1};
    auto sendProgramedVals = toMatrix(sendProgramedVals_u8);

    std::vector<u64> recvSet_u64 = {5, 6, 7, 8, 9, 10, 11, 12};
    auto recvSet = toBlockVec(recvSet_u64);

    std::cout << "####################" << std::endl;

    co::optional<macoro::eager_task<>> senderTask = sender_run(senderSocket, setSize, sendSet, sendProgramedVals) | macoro::make_eager();

    /*
    volePSI::RsPsiSender sender;
    sender.init(8, 8, 40, oc::sysRandomSeed(), false, 1, false);

    co::optional<macoro::eager_task<>> senderTask = sender.run(sendSet, senderSocket) | macoro::make_eager();
    */

    co::optional<macoro::eager_task<>> receiverTask = receiver_run(receiverSocket, setSize, recvSet) | macoro::make_eager();

    /*
    volePSI::RsPsiReceiver receiver;
    receiver.init(8, 8, 40, oc::sysRandomSeed(), false, 1, false);

    co::optional<macoro::eager_task<>> receiverTask = receiver.run(recvSet, receiverSocket) | macoro::make_eager();
    */

    try
    {
        while (receiverTask || senderTask)
        {

            if (receiverTask && receiverTask->is_ready())
            {
                // get the result of the protocol. Might throw.
                sync_wait(*receiverTask);
                std::cout << "recver done" << std::endl;
                receiverTask.reset();
            }

            if (senderTask && senderTask->is_ready())
            {
                // get the result of the protocol. Might throw.
                coproto::sync_wait(*senderTask);
                std::cout << "sender done " << std::endl;
                senderTask.reset();
            }

            coproto::optional<std::vector<oc::u8>> recverMsg = receiverSocket.getOutbound();
            if (recverMsg.has_value())
                senderSocket.processInbound(*recverMsg);

            coproto::optional<std::vector<oc::u8>> senderMsg = senderSocket.getOutbound();
            if (senderMsg)
                receiverSocket.processInbound(*senderMsg);
        }
    }
    catch (std::exception &e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    // sync_wait(when_all_ready(std::move(senderTask), std::move(receiverTask)));

    return 0;
}

void opprf_test(int argc, char **argv)
{
    oc::CLP cmd(argc, argv);

    std::cout << "# Example:" << std::endl;

    messagePassingExampleBoth(cmd);

    std::cout << "# Ours:" << std::endl;

    run(argc, argv);
}