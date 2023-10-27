#pragma once

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "coproto/coproto.h"
#include "coproto/Socket/Socket.h"
#include "coproto/Socket/AsioSocket.h"
#include "macoro/wrap.h"
#include "coproto/Socket/LocalAsyncSock.h"
#include "boost/asio.hpp"
// #include "coproto/Socket/BufferingSocket.h"

#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <tuple>
#include <map>

#include "Defines.h"
#include "sep_opprf.h"

using namespace sepVolePSI;

namespace co = coproto;
using co::Socket, co::task, co::AsioSocket;

namespace ma = macoro;
using ma::eager_task, ma::make_eager, ma::sync_wait, ma::when_all_ready, ma::wrap;

namespace sepVolePSI
{
    class SepPSIPartyAsioSockets
    {
    public:
        // 使わない。shared_ptr用
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZeroSharing;
        std::shared_ptr<AsioSocket> mSendAsioSockForOfflineReconstruction;

        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZeroSharing;
        std::shared_ptr<AsioSocket> mSendAsioSockForOnlineReconstruction;

        explicit SepPSIPartyAsioSockets(
            std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZS,
            std::shared_ptr<AsioSocket> mSendAsioSockForOfflineRC,
            std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZS,
            std::shared_ptr<AsioSocket> mSendAsioSockForOnlineRC);
    };

    class SepPSIParty : public oc::TimerAdapter
    {
    public:
        explicit SepPSIParty(
            std::vector<std::shared_ptr<Socket>> sendSocksForOfflineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOfflineZS,
            std::shared_ptr<Socket> sendSockForOfflineRC,
            std::vector<std::shared_ptr<Socket>> sendSocksForOnlineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOnlineZS,
            std::shared_ptr<Socket> sendSockForOnlineRC,
            u64 myIdx,
            u64 dealerIdx,
            u64 nParties,
            u64 setSize);

        task<> offline();
        task<> online(std::vector<block> set);

        static task<std::string> standaloneWithAsio(
            u64 myIdx,
            u64 dealerIdx,
            u64 nParties,
            // u64 setSize,
            std::vector<block> set,
            std::string host,
            u64 basePortForOfflineZS,
            u64 basePortForOfflineRC,
            u64 basePortForOnlineZS,
            u64 basePortForOnlineRC,
            boost::asio::io_context &io_context,
            bool useTimer = true,
            bool verbose = false);

        std::shared_ptr<SepPSIPartyAsioSockets> mAsioSockets;

    protected:
        task<> offlinePhaseForRC();
        task<> conditionalReconstruction(
            std::vector<block> mSet,
            std::vector<block> zeroShares);
        void setTimersOfOpprfs();

        std::vector<std::shared_ptr<SepOpprfSender>> mOpprfSendersForZeroSharing;
        std::vector<std::shared_ptr<SepOpprfReceiver>> mOpprfReceiversForZeroSharing;
        std::shared_ptr<SepOpprfSender> mOpprfSenderForReconstruction;

        // TODO: forkによってまとめる
        std::vector<std::shared_ptr<Socket>> mSendSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOfflineZeroSharing;
        std::shared_ptr<Socket> mSendSockForOfflineReconstruction;

        std::vector<std::shared_ptr<Socket>> mSendSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOnlineZeroSharing;
        std::shared_ptr<Socket> mSendSockForOnlineReconstruction;

        u64 mMyIdx;
        u64 mDealerIdx;
        u64 mNParties;
        u64 mSetSize;
        PRNG mPrng;

        bool mPrepared = false;
    };

    class SepPSIDealerStandaloneRes
    {
    public:
        std::string mLog;
        std::vector<block> mIntersection;
        explicit SepPSIDealerStandaloneRes(std::string log, std::vector<block> intersection)
            : mLog(std::move(log)),
              mIntersection(std::move(intersection))
        {
        }
    };

    class SepPSIDealerAsioSockets
    {
    public:
        // 使わない。shared_ptr用
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineReconstruction;

        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineReconstruction;

        explicit SepPSIDealerAsioSockets(
            std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineRC,
            std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZS,
            std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineRC);
    };

    class SepPSIDealer : public oc::TimerAdapter
    {
    public:
        explicit SepPSIDealer(
            std::vector<std::shared_ptr<Socket>> sendSocksForOfflineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOfflineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOfflineRC,
            std::vector<std::shared_ptr<Socket>> sendSocksForOnlineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOnlineZS,
            std::vector<std::shared_ptr<Socket>> recvSocksForOnlineRC,
            u64 myIdx,
            u64 nParties,
            u64 setSize);

        task<> offline();
        task<std::vector<block>> online(std::vector<block> set);

        static task<SepPSIDealerStandaloneRes> standaloneWithAsio(
            u64 myIdx,
            u64 nParties,
            std::vector<block> set,
            std::string host,
            u64 basePortForOfflineZS,
            u64 basePortForOfflineRC,
            u64 basePortForOnlineZS,
            u64 basePortForOnlineRC,
            boost::asio::io_context &io_context,
            bool useTimer = true,
            bool verbose = false);

        std::shared_ptr<SepPSIDealerAsioSockets> mAsioSockets;

    protected:
        task<> offlinePhaseForRC();
        task<std::vector<block>> conditionalReconstruction(
            std::vector<block> mSet,
            std::vector<block> zeroShares);
        void setTimersOfOpprfs();

        std::vector<std::shared_ptr<SepOpprfSender>> mOpprfSendersForZeroSharing;
        std::vector<std::shared_ptr<SepOpprfReceiver>> mOpprfReceiversForZeroSharing;
        std::vector<std::shared_ptr<SepOpprfReceiver>> mOpprfReceiversForReconstruction;

        // TODO: forkによってまとめる
        std::vector<std::shared_ptr<Socket>> mSendSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOfflineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOfflineReconstruction;

        std::vector<std::shared_ptr<Socket>> mSendSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOnlineZeroSharing;
        std::vector<std::shared_ptr<Socket>> mRecvSocksForOnlineReconstruction;

        u64 mMyIdx;
        u64 mNParties;
        u64 mSetSize;
        PRNG mPrng;

        bool mPrepared = false;
    };

    class SepPSI : public oc::TimerAdapter
    {
    public:
        explicit SepPSI(
            SepPSIDealer dealer,
            std::map<u64, SepPSIParty> parties,
            u64 nParties,
            u64 dealerIdx);
        static std::vector<block> run(SepPSI psi, u64 nParties, std::vector<std::vector<block>> sets);
        // static std::vector<block> runWithAsioSocket(u64 nParties, std::vector<std::vector<block>> sets);
        static std::vector<block> runWithLocalAsyncSocket(u64 nParties, std::vector<std::vector<block>> sets);

    protected:
        static SepPSI init(
            u64 nParties, u64 setSize,
            std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForOffZS,
            std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForOffZS,
            std::vector<std::shared_ptr<Socket>> sendSocksForOffRC,
            std::vector<std::shared_ptr<Socket>> recvSocksForOffRC,
            std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForOnZS,
            std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForOnZS,
            std::vector<std::shared_ptr<Socket>> sendSocksForOnRC,
            std::vector<std::shared_ptr<Socket>> recvSocksForOnRC);
        static SepPSI initWithLocalAsyncSocket(u64 nParties, u64 setSize);
        // static SepPSI initWithAsioSocket(u64 nParties, u64 setSize);
        task<> offline(std::vector<std::shared_ptr<oc::Timer>> timers);
        task<std::vector<block>> online(std::vector<std::vector<block>> sets, std::vector<std::shared_ptr<oc::Timer>> timers);

        SepPSIDealer mDealer;
        std::map<u64, SepPSIParty> mParties;
        u64 mNParties;
        u64 mDealerIdx;
    };

    void sep_psi_3party_test(int argc, char **argv);

    void timer_summarization(std::vector<std::shared_ptr<oc::Timer>> timers);
}