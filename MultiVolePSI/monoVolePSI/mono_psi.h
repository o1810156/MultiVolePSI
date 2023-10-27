#pragma once

#include "cryptoTools/Common/Defines.h"
// #include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "coproto/coproto.h"
#include "coproto/Socket/Socket.h"
#include "coproto/Socket/AsioSocket.h"
#include <future>
#include "boost/asio.hpp"
// #include "coproto/Socket/BufferingSocket.h"
#include "volePSI/RsOpprf.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <map>
#include <sstream>

#include "Defines.h"

namespace oc = osuCrypto;
using oc::u64, oc::u32, oc::u8, oc::u8,
    oc::block,
    oc::PRNG,
    oc::span,
    oc::ZeroBlock;

namespace co = coproto;
using co::Socket, co::task;

namespace ma = macoro;
using ma::eager_task, ma::make_eager, ma::sync_wait, ma::when_all_ready, ma::wrap;

/*
namespace monopsi {
    task<> party(
        u64 myIdx,
        u64 dealerIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocks,
        std::vector<std::shared_ptr<Socket>> recvSocks);

    task<std::vector<block>> party_dealer(
        u64 myIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocks,
        std::vector<std::shared_ptr<Socket>> recvSocks);

    int mono_psi_main(int argc, char **argv);
}
*/

namespace monoVolePSI
{
    class MonoPSIParty : public oc::TimerAdapter
    {
    public:
        explicit MonoPSIParty(
            std::vector<std::shared_ptr<Socket>> sendSocksForZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForZS,
            std::shared_ptr<Socket> sendSockForRC,
            u64 myIdx,
            u64 dealerIdx,
            u64 nParties,
            u64 setSize);

        task<> online(std::vector<block> set);

        static task<std::string> standaloneWithAsio(
            u64 myIdx,
            u64 dealerIdx,
            u64 nParties,
            // u64 setSize,
            std::vector<block> set,
            std::string host,
            u64 basePortForZS,
            u64 basePortForRC,
            u64 basePortForOnlineZS, // dummy
            u64 basePortForOnlineRC, // dummy
            boost::asio::io_context &io_context,
            bool useTimer = true,
            bool verbose = false);

    protected:
        task<> conditionalReconstruction(
            std::vector<block> mSet,
            std::vector<block> zeroShares);
        void setTimersOfOpprfs();

        std::vector<std::shared_ptr<volePSI::RsOpprfSender>> mOpprfSendersForZeroSharing;
        std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>> mOpprfReceiversForZeroSharing;
        std::shared_ptr<volePSI::RsOpprfSender> mOpprfSenderForReconstruction;

        std::vector<std::shared_ptr<Socket>> mSendSocksForZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForZeroSharing;
        std::shared_ptr<Socket> mSendSockForReconstruction;

        u64 mMyIdx;
        u64 mDealerIdx;
        u64 mNParties;
        u64 mSetSize;
        PRNG mPrng;
    };

    class MonoPSIDealerStandaloneRes
    {
    public:
        std::string mLog;
        std::vector<block> mIntersection;
        explicit MonoPSIDealerStandaloneRes(std::string log, std::vector<block> intersection)
            : mLog(std::move(log)),
              mIntersection(std::move(intersection))
        {
        }
    };

    class MonoPSIDealer : public oc::TimerAdapter
    {
    public:
        explicit MonoPSIDealer(
            std::vector<std::shared_ptr<Socket>> sendSocksForZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForRC,
            u64 myIdx,
            u64 nParties,
            u64 setSize);

        task<std::vector<block>> online(std::vector<block> set);

        static task<MonoPSIDealerStandaloneRes> standaloneWithAsio(
            u64 myIdx,
            u64 nParties,
            std::vector<block> set,
            std::string host,
            u64 basePortForZS,
            u64 basePortForRC,
            u64 basePortForOnlineZS, // dummy
            u64 basePortForOnlineRC, // dummy
            boost::asio::io_context &io_context,
            bool useTimer = true,
            bool verbose = false);

    protected:
        task<std::vector<block>> conditionalReconstruction(
            std::vector<block> mSet,
            std::vector<block> zeroShares);
        void setTimersOfOpprfs();

        std::vector<std::shared_ptr<volePSI::RsOpprfSender>> mOpprfSendersForZeroSharing;
        std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>> mOpprfReceiversForZeroSharing;
        std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>> mOpprfReceiversForReconstruction;

        // TODO: forkによってまとめる
        std::vector<std::shared_ptr<Socket>> mSendSocksForZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForReconstruction;

        u64 mMyIdx;
        u64 mNParties;
        u64 mSetSize;
        PRNG mPrng;
    };

    class MonoPSI : public oc::TimerAdapter
    {
    public:
        explicit MonoPSI(
            MonoPSIDealer dealer,
            std::map<u64, MonoPSIParty> parties,
            u64 nParties,
            u64 dealerIdx);
        static std::vector<block> run(MonoPSI psi, u64 nParties, std::vector<std::vector<block>> sets);
        static std::vector<block> runWithLocalAsyncSocket(u64 nParties, std::vector<std::vector<block>> sets);
        // static std::vector<block> runWithAsioSocket(u64 nParties, std::vector<std::vector<block>> sets);
    protected:
        static MonoPSI initWithLocalAsyncSocket(u64 nParties, u64 setSize);
        // static MonoPSI initWithAsioSocket(u64 nParties, u64 setSize);
        static MonoPSI init(
            u64 nParties, u64 setSize,
            std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForOffZS,
            std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForOffZS,
            std::vector<std::shared_ptr<Socket>> sendSocksForOffRC,
            std::vector<std::shared_ptr<Socket>> recvSocksForOffRC);
        task<std::vector<block>> online(std::vector<std::vector<block>> sets, std::vector<std::shared_ptr<oc::Timer>> timers);

        MonoPSIDealer mDealer;
        std::map<u64, MonoPSIParty> mParties;
        u64 mNParties;
        u64 mDealerIdx;
    };

    void mono_psi_3party_test(int argc, char **argv);

    void timer_summarization(std::vector<std::shared_ptr<oc::Timer>> timers);
}