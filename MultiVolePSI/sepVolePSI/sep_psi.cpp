#include "sep_psi.h"
#include "utils.h"
#include "sockets/sockets.h"
#include <chrono>

namespace sepVolePSI
{
    oc::MatrixView<u8> span2matrix(span<block> vec)
    {
        return oc::MatrixView<u8>((u8 *)vec.data(), vec.size(), 16);
    }

    std::vector<block> matrix2vec(oc::MatrixView<u8> m)
    {
        return std::vector<block>((block *)m.begin(), (block *)m.end());
    }

    /*
    task<> dummy_task()
    {
        MC_BEGIN(task<>);
        MC_END();
    }
    */

    template <class T>
    task<> offlinePhaseForZS(
        T &timer,
        u64 myIdx,
        u64 nParties,
        u64 setSize,
        std::vector<std::shared_ptr<Socket>> sendSocks,
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfs,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfs,
        PRNG &prng)
    {
        MC_BEGIN(task<>, &timer, myIdx, nParties, setSize, sendSocks, recvSocks, sendOpprfs, recvOpprfs, &prng,
                 numThreads = u64(1),
                 sendTasks = std::vector<eager_task<>>(nParties),
                 recvTasks = std::vector<eager_task<>>(nParties),
                 idxTask = u64(0));

        timer.setTimePoint("[" + std::to_string(myIdx) + "] offlinePhaseForZS_start");

        for (u64 i = 0; i < nParties; i++)
        {
            if (i == myIdx)
            {
                continue;
            }

            sendTasks[i] = sendOpprfs[i]->offline_send(setSize, prng, numThreads, *sendSocks[i], myIdx) | make_eager();
            recvTasks[i] = recvOpprfs[i]->offline_receive(setSize, prng, numThreads, *recvSocks[i], myIdx) | make_eager();
        }
        for (idxTask = 0; idxTask < nParties; idxTask++)
        {
            if (idxTask == myIdx)
            {
                continue;
            }

            MC_AWAIT(sendTasks[idxTask]);
            MC_AWAIT(recvTasks[idxTask]);
        }

        timer.setTimePoint("[" + std::to_string(myIdx) + "] offlinePhaseForZS_end");

        MC_END();
    }

    template <class T>
    task<std::vector<block>> conditionalZeroSharing(
        T &timer,
        u64 myIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocks, // myIdx番目(自分宛)のも便宜上含む。つまり len(sendSocks) = nParties である
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<SepOpprfSender>> opprfSenders,
        std::vector<std::shared_ptr<SepOpprfReceiver>> opprfReceivers,
        PRNG &prng)
    {
        MC_BEGIN(
            task<std::vector<block>>,
            &timer,
            myIdx,
            setSize = u64(mSet.size()),
            mSet,
            nParties,
            sendSocks,
            recvSocks,
            // psiSecParam = u64(40),
            // bitSize = u64(128),
            numThreads = u64(1),
            &prng,
            // std::vector<std::vector<block>> sendShares(nParties), recvPayLoads(nParties); // original
            sendShares = std::vector<std::vector<block>>(nParties),
            recvShares = std::vector<std::vector<block>>(nParties),
            opprfSenders,
            opprfReceivers,
            sendTasks = std::vector<eager_task<>>(nParties),
            recvTasks = std::vector<eager_task<>>(nParties),
            idxTask = u64(0),
            result = std::vector<block>(u64(mSet.size())));

        timer.setTimePoint("[" + std::to_string(myIdx) + "] conditional_zero_sharing_start");

        for (u64 idxP = 0; idxP < nParties; ++idxP)
        {
            sendShares[idxP].resize(setSize);
            recvShares[idxP].resize(setSize);
        }

        for (u64 i = 0; i < setSize; ++i)
        {

            block sum = ZeroBlock;

            for (u64 idxP = 0; idxP < nParties; ++idxP)
            {
                if (idxP == myIdx)
                {
                    continue;
                }

                sendShares[idxP][i] = prng.get<block>();
                sum = sum ^ sendShares[idxP][i];
            }

            sendShares[myIdx][i] = sum;
        }

        timer.setTimePoint("[" + std::to_string(myIdx) + "] conditional_zero_sharing_sending_shares_start");

        for (u64 idxP = 0; idxP < nParties; idxP++)
        {
            if (idxP == myIdx)
            {
                // sendTasks[idxP] = dummy_task() | macoro::make_eager();
                // recvTasks[idxP] = dummy_task() | macoro::make_eager();
                continue;
            }

            sendTasks[idxP] = opprfSenders[idxP]->online_send(setSize, mSet, span2matrix(sendShares[idxP]), prng, numThreads, *sendSocks[idxP]) | macoro::make_eager();
            recvTasks[idxP] = opprfReceivers[idxP]->online_receive(setSize, mSet, span2matrix(recvShares[idxP]), prng, numThreads, *recvSocks[idxP]) | macoro::make_eager();
        }

        for (idxTask = 0; idxTask < nParties; idxTask++)
        {
            if (idxTask == myIdx)
            {
                continue;
            }

            MC_AWAIT(sendTasks[idxTask]);
            MC_AWAIT(recvTasks[idxTask]);
        }

        timer.setTimePoint("[" + std::to_string(myIdx) + "] conditional_zero_sharing_sending_shares_end");

        for (u64 i = 0; i < setSize; ++i)
        {
            block sum = ZeroBlock;

            for (u64 idxP = 0; idxP < nParties; ++idxP)
            {
                if (idxP == myIdx)
                {
                    sum = sum ^ sendShares[idxP][i];
                }

                sum = sum ^ recvShares[idxP][i];
            }

            result[i] = sum;
        }

        timer.setTimePoint("[" + std::to_string(myIdx) + "] conditional_zero_sharing_end");

        MC_RETURN(result);

        MC_END();
    }

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

    // SockType example: co::LocalAsyncSocket
    template <class SockType>
    std::vector<std::shared_ptr<Socket>> toSocketVec(std::vector<std::shared_ptr<SockType>> vec)
    {
        std::vector<std::shared_ptr<Socket>> ret(vec.size());
        for (u64 i = 0; i < vec.size(); ++i)
        {
            ret[i] = vec[i];
        }
        return ret;
    }

    template <class SockType>
    std::vector<std::vector<std::shared_ptr<Socket>>> toSocketVecVec(std::vector<std::vector<std::shared_ptr<SockType>>> vec)
    {
        std::vector<std::vector<std::shared_ptr<Socket>>> ret(vec.size());
        for (u64 i = 0; i < vec.size(); ++i)
        {
            ret[i] = toSocketVec<SockType>(vec[i]);
        }
        return ret;
    }

    /*
    std::tuple<
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>,
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>>
    createLASocketsForZS(int n)
    {
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>> sendSocks(n);
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            sendSocks[i] = std::vector<std::shared_ptr<co::LocalAsyncSocket>>(n);
            recvSocks[i] = std::vector<std::shared_ptr<co::LocalAsyncSocket>>(n);
        }

        for (auto i = 0; i < n; i++)
        {
            for (auto j = 0; j < n; j++)
            {
                auto p = co::LocalAsyncSocket::makePair();
                sendSocks[i][j] = std::make_shared<co::LocalAsyncSocket>(p[0]);
                recvSocks[j][i] = std::make_shared<co::LocalAsyncSocket>(p[1]);
            }
        }

        return std::tuple<
            std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>,
            std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>>(sendSocks, recvSocks);
    }

    std::tuple<
        std::vector<std::shared_ptr<co::LocalAsyncSocket>>,
        std::vector<std::shared_ptr<co::LocalAsyncSocket>>>
    createLASocketsForRC(int n)
    {
        std::vector<std::shared_ptr<co::LocalAsyncSocket>> sendSocks(n);
        std::vector<std::shared_ptr<co::LocalAsyncSocket>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            auto p = co::LocalAsyncSocket::makePair();
            sendSocks[i] = std::make_shared<co::LocalAsyncSocket>(p[0]);
            recvSocks[i] = std::make_shared<co::LocalAsyncSocket>(p[1]);
        }

        return std::tuple<
            std::vector<std::shared_ptr<co::LocalAsyncSocket>>,
            std::vector<std::shared_ptr<co::LocalAsyncSocket>>>(sendSocks, recvSocks);
    }
    */

    SepPSIPartyAsioSockets::SepPSIPartyAsioSockets(
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZS,
        std::shared_ptr<AsioSocket> mSendAsioSockForOfflineRC,
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZS,
        std::shared_ptr<AsioSocket> mSendAsioSockForOnlineRC)
        : mSendAsioSocksForOfflineZeroSharing(mSendAsioSocksForOfflineZS),
          mRecvAsioSocksForOfflineZeroSharing(mRecvAsioSocksForOfflineZS),
          mSendAsioSockForOfflineReconstruction(mSendAsioSockForOfflineRC),
          mSendAsioSocksForOnlineZeroSharing(mSendAsioSocksForOnlineZS),
          mRecvAsioSocksForOnlineZeroSharing(mRecvAsioSocksForOnlineZS),
          mSendAsioSockForOnlineReconstruction(mSendAsioSockForOnlineRC)
    {
    }

    SepPSIDealerAsioSockets::SepPSIDealerAsioSockets(
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOfflineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOfflineRC,
        std::vector<std::shared_ptr<AsioSocket>> mSendAsioSocksForOnlineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineZS,
        std::vector<std::shared_ptr<AsioSocket>> mRecvAsioSocksForOnlineRC)
        : mSendAsioSocksForOfflineZeroSharing(mSendAsioSocksForOfflineZS),
          mRecvAsioSocksForOfflineZeroSharing(mRecvAsioSocksForOfflineZS),
          mRecvAsioSocksForOfflineReconstruction(mRecvAsioSocksForOfflineRC),
          mSendAsioSocksForOnlineZeroSharing(mSendAsioSocksForOnlineZS),
          mRecvAsioSocksForOnlineZeroSharing(mRecvAsioSocksForOnlineZS),
          mRecvAsioSocksForOnlineReconstruction(mRecvAsioSocksForOnlineRC)
    {
    }

    SepPSIParty::SepPSIParty(
        std::vector<std::shared_ptr<Socket>> sendSocksForOfflineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOfflineZS,
        std::shared_ptr<Socket> sendSockForOfflineRC,
        std::vector<std::shared_ptr<Socket>> sendSocksForOnlineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOnlineZS,
        std::shared_ptr<Socket> sendSockForOnlineRC,
        u64 myIdx,
        u64 dealerIdx,
        u64 nParties,
        u64 setSize)
        : mSendSocksForOfflineZeroSharing(sendSocksForOfflineZS),
          mRecvSocksForOfflineZeroSharing(recvSocksForOfflineZS),
          mSendSockForOfflineReconstruction(sendSockForOfflineRC),
          mSendSocksForOnlineZeroSharing(sendSocksForOnlineZS),
          mRecvSocksForOnlineZeroSharing(recvSocksForOnlineZS),
          mSendSockForOnlineReconstruction(sendSockForOnlineRC),
          mMyIdx(myIdx),
          mDealerIdx(dealerIdx),
          mNParties(nParties),
          mSetSize(setSize),
          mPrng(PRNG(_mm_set_epi32(4253465, 3434565, myIdx, myIdx)))
    {
        mOpprfSendersForZeroSharing = std::vector<std::shared_ptr<SepOpprfSender>>(nParties);
        mOpprfReceiversForZeroSharing = std::vector<std::shared_ptr<SepOpprfReceiver>>(nParties);
        mOpprfSenderForReconstruction = std::make_shared<SepOpprfSender>();

        for (u64 i = 0; i < nParties; ++i)
        {
            mOpprfSendersForZeroSharing[i] = std::make_shared<SepOpprfSender>();
            mOpprfReceiversForZeroSharing[i] = std::make_shared<SepOpprfReceiver>();
        }
    }

    task<> SepPSIParty::offlinePhaseForRC()
    {
        MC_BEGIN(task<>, this,
                 numThreads = u64(1));

        setTimePoint("[" + std::to_string(mMyIdx) + "] offlinePhaseForRC_child_start");

        MC_AWAIT(mOpprfSenderForReconstruction->offline_send(mSetSize, mPrng, numThreads, *mSendSockForOfflineReconstruction));

        setTimePoint("[" + std::to_string(mMyIdx) + "] offlinePhaseForRC_child_end");

        MC_END();
    }

    void SepPSIParty::setTimersOfOpprfs()
    {
        if (mTimer)
        {
            for (u64 i = 0; i < mNParties; i++)
            {
                mOpprfSendersForZeroSharing[i]->setTimer(*mTimer);
                mOpprfReceiversForZeroSharing[i]->setTimer(*mTimer);
            }
            mOpprfSenderForReconstruction->setTimer(*mTimer);
        }
    }

    task<> SepPSIParty::offline()
    {
        MC_BEGIN(task<>,
                 this);

        setTimersOfOpprfs();

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_offline_start");

        MC_AWAIT(offlinePhaseForZS<SepPSIParty>(
            *this,
            mMyIdx, mNParties, mSetSize,
            mSendSocksForOfflineZeroSharing,
            mRecvSocksForOfflineZeroSharing,
            mOpprfSendersForZeroSharing,
            mOpprfReceiversForZeroSharing,
            mPrng));

        MC_AWAIT(offlinePhaseForRC());

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_offline_end");

        mPrepared = true;

        MC_END();
    }

    task<> SepPSIParty::conditionalReconstruction(
        std::vector<block> mSet,
        std::vector<block> zeroShares)
    {
        MC_BEGIN(task<>,
                 this,
                 mSet,
                 zeroShares,
                 numThreads = u64(1));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_child_start");

        MC_AWAIT(mOpprfSenderForReconstruction->online_send(mSetSize, mSet, span2matrix(zeroShares), mPrng, numThreads, *mSendSockForOnlineReconstruction));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_child_end");

        MC_END();
    }

    task<> SepPSIParty::online(std::vector<block> set)
    {
        MC_BEGIN(task<>,
                 this,
                 set,
                 zeroShares = std::vector<block>(mNParties));

        setTimersOfOpprfs();

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_online_start");

        MC_AWAIT_SET(zeroShares, conditionalZeroSharing<SepPSIParty>(
                                     *this,
                                     mMyIdx, set, mNParties,
                                     mSendSocksForOnlineZeroSharing,
                                     mRecvSocksForOnlineZeroSharing,
                                     mOpprfSendersForZeroSharing,
                                     mOpprfReceiversForZeroSharing,
                                     mPrng));

        MC_AWAIT(conditionalReconstruction(set, zeroShares));

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_online_end");

        MC_END();
    }

    task<std::string> SepPSIParty::standaloneWithAsio(
        u64 myIdx,
        u64 dealerIdx,
        u64 nParties,
        std::vector<block> set,
        std::string host,
        u64 basePortForOfflineZS,
        u64 basePortForOfflineRC,
        u64 basePortForOnlineZS,
        u64 basePortForOnlineRC,
        boost::asio::io_context &io_context,
        bool useTimer,
        bool verbose)
    {
        MC_BEGIN(task<std::string>,
                 myIdx,
                 dealerIdx,
                 nParties,
                 set,
                 host,
                 basePortForOfflineZS,
                 basePortForOfflineRC,
                 basePortForOnlineZS,
                 basePortForOnlineRC,
                 &io_context,
                 useTimer,
                 verbose,
                 wholeTimer = std::shared_ptr<oc::Timer>(),
                 offlineTimer = std::shared_ptr<oc::Timer>(),
                 onlineTimer = std::shared_ptr<oc::Timer>(),
                 tmpSock = AsioSocket(),
                 port = u64(0),
                 sendSocksForOfflineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOfflineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 sendSockForOfflineRC = std::make_shared<AsioSocket>(),
                 sendSocksForOnlineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOnlineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 sendSockForOnlineRC = std::make_shared<AsioSocket>(),
                 asioSocks = std::shared_ptr<SepPSIPartyAsioSockets>(),
                 sockIdx = u64(0),
                 i = int(0),
                 party = std::shared_ptr<SepPSIParty>(),
                 oss = std::ostringstream());

        if (verbose)
        {
            oss << "myItems: " << std::endl;
            for (auto item : set)
            {
                oss << item << std::endl;
            }
        }

        for (sockIdx = nParties - 1; sockIdx > myIdx; --sockIdx)
        {
            port = basePortForOfflineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                continue;
            }

            port = basePortForOfflineZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "<-" << sockIdx << "]"
                << "accepted at " << host << ":" << port << std::endl;
        }

        for (i = int(myIdx) - 1; i >= 0; --i)
        {
            sockIdx = u64(i);

            port = basePortForOfflineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
        }

        port = basePortForOfflineRC + (myIdx * nParties + dealerIdx);
        MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
        sendSockForOfflineRC = std::make_shared<AsioSocket>(std::move(tmpSock));

        port = basePortForOnlineRC + (myIdx * nParties + dealerIdx);
        MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
        sendSockForOnlineRC = std::make_shared<AsioSocket>(std::move(tmpSock));

        oss << "(RC) connect to " << host << ":" << port << std::endl;

        asioSocks = std::make_shared<SepPSIPartyAsioSockets>(
            SepPSIPartyAsioSockets(
                sendSocksForOfflineZS,
                recvSocksForOfflineZS,
                sendSockForOfflineRC,
                sendSocksForOnlineZS,
                recvSocksForOnlineZS,
                sendSockForOnlineRC));

        party = std::make_shared<SepPSIParty>(SepPSIParty(
            toSocketVec<AsioSocket>(sendSocksForOfflineZS),
            toSocketVec<AsioSocket>(recvSocksForOfflineZS),
            sendSockForOfflineRC,
            toSocketVec<AsioSocket>(sendSocksForOnlineZS),
            toSocketVec<AsioSocket>(recvSocksForOnlineZS),
            sendSockForOnlineRC,
            myIdx,
            dealerIdx,
            nParties,
            u64(set.size())));

        party->mAsioSockets = asioSocks;

        wholeTimer = std::make_shared<oc::Timer>();
        offlineTimer = std::make_shared<oc::Timer>();
        onlineTimer = std::make_shared<oc::Timer>();

        if (useTimer)
        {
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_start");
            party->setTimer(*offlineTimer);
        }

        MC_AWAIT(party->offline());

        if (useTimer)
        {
            party->setTimer(*onlineTimer);
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_mid");
        }

        MC_AWAIT(party->online(set));

        if (useTimer)
        {
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_end");
            oss << "=== Time Summarization [" << myIdx << "] ===" << std::endl;
            oss << *offlineTimer << std::endl;
            oss << *onlineTimer << std::endl;
            oss << *wholeTimer << std::endl;
        }

        MC_RETURN(oss.str());

        MC_END();
    }

    SepPSIDealer::SepPSIDealer(
        std::vector<std::shared_ptr<Socket>> sendSocksForOfflineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOfflineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOfflineRC,
        std::vector<std::shared_ptr<Socket>> sendSocksForOnlineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOnlineZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForOnlineRC,
        u64 myIdx,
        u64 nParties,
        u64 setSize)
        : mSendSocksForOfflineZeroSharing(sendSocksForOfflineZS),
          mRecvSocksForOfflineZeroSharing(recvSocksForOfflineZS),
          mRecvSocksForOfflineReconstruction(recvSocksForOfflineRC),
          mSendSocksForOnlineZeroSharing(sendSocksForOnlineZS),
          mRecvSocksForOnlineZeroSharing(recvSocksForOnlineZS),
          mRecvSocksForOnlineReconstruction(recvSocksForOnlineRC),
          mMyIdx(myIdx),
          mNParties(nParties),
          mSetSize(setSize),
          mPrng(PRNG(_mm_set_epi32(4253465, 3434565, myIdx, myIdx)))
    {

        mOpprfSendersForZeroSharing = std::vector<std::shared_ptr<SepOpprfSender>>(nParties);
        mOpprfReceiversForZeroSharing = std::vector<std::shared_ptr<SepOpprfReceiver>>(nParties);
        mOpprfReceiversForReconstruction = std::vector<std::shared_ptr<SepOpprfReceiver>>(nParties);

        for (u64 i = 0; i < nParties; ++i)
        {
            mOpprfSendersForZeroSharing[i] = std::make_shared<SepOpprfSender>();
            mOpprfReceiversForZeroSharing[i] = std::make_shared<SepOpprfReceiver>();
            mOpprfReceiversForReconstruction[i] = std::make_shared<SepOpprfReceiver>();
        }
    }

    task<> SepPSIDealer::offlinePhaseForRC()
    {
        MC_BEGIN(task<>, this,
                 numThreads = u64(1),
                 recvTasks = std::vector<eager_task<>>(mNParties),
                 idxTask = u64(0));

        setTimePoint("[" + std::to_string(mMyIdx) + "] offlinePhaseForRC_dealer_start");

        for (u64 i = 0; i < mNParties; i++)
        {
            if (i == mMyIdx)
            {
                continue;
            }

            recvTasks[i] = mOpprfReceiversForReconstruction[i]->offline_receive(mSetSize, mPrng, numThreads, *mRecvSocksForOfflineReconstruction[i]) | macoro::make_eager();
        }

        for (idxTask = 0; idxTask < mNParties; idxTask++)
        {
            if (idxTask == mMyIdx)
            {
                continue;
            }

            MC_AWAIT(recvTasks[idxTask]);
        }

        setTimePoint("[" + std::to_string(mMyIdx) + "] offlinePhaseForRC_dealer_end");

        MC_END();
    }

    void SepPSIDealer::setTimersOfOpprfs()
    {
        if (mTimer)
        {
            for (u64 i = 0; i < mNParties; i++)
            {
                mOpprfSendersForZeroSharing[i]->setTimer(*mTimer);
                mOpprfReceiversForZeroSharing[i]->setTimer(*mTimer);
                mOpprfReceiversForReconstruction[i]->setTimer(*mTimer);
            }
        }
    }

    task<> SepPSIDealer::offline()
    {
        MC_BEGIN(task<>, this);

        setTimersOfOpprfs();

        // print_debug(mMyIdx, "party_offline_dealer", "start");
        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_offline_start");

        MC_AWAIT(offlinePhaseForZS<SepPSIDealer>(
            *this,
            mMyIdx, mNParties, mSetSize,
            mSendSocksForOfflineZeroSharing,
            mRecvSocksForOfflineZeroSharing,
            mOpprfSendersForZeroSharing,
            mOpprfReceiversForZeroSharing,
            mPrng));

        // print_debug(mMyIdx, "party_dealer", "offlinePhaseForZS done");

        MC_AWAIT(offlinePhaseForRC());

        // print_debug(mMyIdx, "party_dealer", "offlinePhaseForRC_dealer done");
        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_offline_end");

        mPrepared = true;

        MC_END();
    }

    task<std::vector<block>> SepPSIDealer::conditionalReconstruction(
        std::vector<block> mSet,
        std::vector<block> zeroShares)
    {
        MC_BEGIN(
            task<std::vector<block>>,
            this,
            mSet,
            zeroShares,
            numThreads = u64(1),
            recvShares = std::vector<std::vector<block>>(mNParties),
            recvTasks = std::vector<eager_task<>>(mNParties),
            idxTask = u64(0),
            intersection = std::vector<block>(0));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_dealer_start");

        for (u64 idxP = 0; idxP < mNParties; ++idxP)
        {
            recvShares[idxP].resize(mSetSize);
        }

        for (u64 idxP = 0; idxP < mNParties; idxP++)
        {
            if (idxP == mMyIdx)
            {
                // recvTasks[idxP] = dummy_task() | macoro::make_eager();
                continue;
            }

            recvTasks[idxP] = mOpprfReceiversForReconstruction[idxP]->online_receive(mSetSize, mSet, span2matrix(recvShares[idxP]), mPrng, numThreads, *mRecvSocksForOnlineReconstruction[idxP]) | macoro::make_eager();
        }

        for (idxTask = 0; idxTask < mNParties; idxTask++)
        {
            if (idxTask == mMyIdx)
            {
                continue;
            }

            MC_AWAIT(recvTasks[idxTask]);
        }

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_dealer_receiving_shares_end");

        for (u64 i = 0; i < mSetSize; ++i)
        {
            block sum = ZeroBlock;

            for (u64 idxP = 0; idxP < mNParties; ++idxP)
            {
                if (idxP == mMyIdx)
                {
                    sum = sum ^ zeroShares[i];
                }

                sum = sum ^ recvShares[idxP][i];
            }

            if (sum == ZeroBlock)
            {
                intersection.push_back(mSet[i]);
            }
        }

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_dealer_end");

        MC_RETURN(intersection);

        MC_END();
    }

    task<std::vector<block>> SepPSIDealer::online(std::vector<block> set)
    {
        MC_BEGIN(task<std::vector<block>>,
                 this,
                 set,
                 zeroShares = std::vector<block>(mNParties),
                 intersection = std::vector<block>(0));

        setTimersOfOpprfs();

        // print_debug(mMyIdx, "party_online_dealer", "start");
        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_online_start");

        MC_AWAIT_SET(zeroShares, conditionalZeroSharing<SepPSIDealer>(
                                     *this,
                                     mMyIdx, set, mNParties,
                                     mSendSocksForOnlineZeroSharing,
                                     mRecvSocksForOnlineZeroSharing,
                                     mOpprfSendersForZeroSharing,
                                     mOpprfReceiversForZeroSharing,
                                     mPrng));

        // print_debug(mMyIdx, "party_dealer", "conditional_zero_sharing done");

        MC_AWAIT_SET(intersection, conditionalReconstruction(set, zeroShares));

        // print_debug(mMyIdx, "party_dealer", "conditional_reconstruction_dealer done");
        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_online_end");

        MC_RETURN(intersection);

        MC_END();
    }

    task<SepPSIDealerStandaloneRes> SepPSIDealer::standaloneWithAsio(
        u64 myIdx,
        u64 nParties,
        std::vector<block> set,
        std::string host,
        u64 basePortForOfflineZS,
        u64 basePortForOfflineRC,
        u64 basePortForOnlineZS,
        u64 basePortForOnlineRC,
        boost::asio::io_context &io_context,
        bool useTimer,
        bool verbose)
    {
        MC_BEGIN(task<SepPSIDealerStandaloneRes>,
                 myIdx,
                 nParties,
                 set,
                 host,
                 basePortForOfflineZS,
                 basePortForOfflineRC,
                 basePortForOnlineZS,
                 basePortForOnlineRC,
                 &io_context,
                 useTimer,
                 verbose,
                 wholeTimer = std::shared_ptr<oc::Timer>(),
                 offlineTimer = std::shared_ptr<oc::Timer>(),
                 onlineTimer = std::shared_ptr<oc::Timer>(),
                 tmpSock = AsioSocket(),
                 port = u64(0),
                 sendSocksForOfflineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOfflineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOfflineRC = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 sendSocksForOnlineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOnlineZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 recvSocksForOnlineRC = std::vector<std::shared_ptr<AsioSocket>>(nParties),
                 asioSocks = std::shared_ptr<SepPSIDealerAsioSockets>(),
                 sockIdx = u64(0),
                 i = int(0),
                 party = std::shared_ptr<SepPSIDealer>(),
                 intersection = std::vector<block>(0),
                 oss = std::ostringstream());

        if (verbose)
        {
            oss << "myItems: " << std::endl;
            for (auto item : set)
            {
                oss << item << std::endl;
            }
        }

        for (sockIdx = nParties - 1; sockIdx > myIdx; --sockIdx)
        {
            port = basePortForOfflineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                continue;
            }

            port = basePortForOfflineZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "<-" << sockIdx << "]"
                << "accepted at " << host << ":" << port << std::endl;
        }

        for (i = int(myIdx) - 1; i >= 0; --i)
        {
            sockIdx = u64(i);

            port = basePortForOfflineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOfflineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForOnlineZS[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                continue;
            }

            port = basePortForOfflineRC + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOfflineRC[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            port = basePortForOnlineRC + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForOnlineRC[sockIdx] = std::make_shared<AsioSocket>(std::move(tmpSock));

            oss << "(RC) accept at " << host << ":" << port << std::endl;
        }

        asioSocks = std::make_shared<SepPSIDealerAsioSockets>(
            SepPSIDealerAsioSockets(
                sendSocksForOfflineZS,
                recvSocksForOfflineZS,
                recvSocksForOfflineRC,
                sendSocksForOnlineZS,
                recvSocksForOnlineZS,
                recvSocksForOnlineRC));

        party = std::make_shared<SepPSIDealer>(SepPSIDealer(
            toSocketVec<AsioSocket>(sendSocksForOfflineZS),
            toSocketVec<AsioSocket>(recvSocksForOfflineZS),
            toSocketVec<AsioSocket>(recvSocksForOfflineRC),
            toSocketVec<AsioSocket>(sendSocksForOnlineZS),
            toSocketVec<AsioSocket>(recvSocksForOnlineZS),
            toSocketVec<AsioSocket>(recvSocksForOnlineRC),
            myIdx,
            nParties,
            u64(set.size())));

        party->mAsioSockets = asioSocks;

        std::cout << "SepPSIDealer Prepared." << std::endl;

        wholeTimer = std::make_shared<oc::Timer>();
        offlineTimer = std::make_shared<oc::Timer>();
        onlineTimer = std::make_shared<oc::Timer>();

        if (useTimer)
        {
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_start");
            party->setTimer(*offlineTimer);
        }

        std::cout << "SepPSIDealer Offline Start." << std::endl;

        MC_AWAIT(party->offline());

        if (useTimer)
        {
            party->setTimer(*onlineTimer);
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_mid");
        }

        std::cout << "SepPSIDealer Online Start." << std::endl;

        MC_AWAIT_SET(intersection, party->online(set));

        if (useTimer)
        {
            wholeTimer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_end");
            oss << "=== Time Summarization [" << myIdx << "] ===" << std::endl;
            oss << *offlineTimer << std::endl;
            oss << *onlineTimer << std::endl;
            oss << *wholeTimer << std::endl;
        }

        std::cout << "SepPSIDealer Done." << std::endl;

        MC_RETURN(SepPSIDealerStandaloneRes(oss.str(), intersection));

        MC_END();
    }

    SepPSI::SepPSI(
        SepPSIDealer dealer,
        std::map<u64, SepPSIParty> parties,
        u64 nParties,
        u64 dealerIdx)
        : mDealer(std::move(dealer)),
          mParties(std::move(parties)),
          mNParties(nParties),
          mDealerIdx(dealerIdx)
    {
    }

    SepPSI SepPSI::init(
        u64 nParties, u64 setSize,
        std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForOffZS,
        std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForOffZS,
        std::vector<std::shared_ptr<Socket>> sendSocksForOffRC,
        std::vector<std::shared_ptr<Socket>> recvSocksForOffRC,
        std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForOnZS,
        std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForOnZS,
        std::vector<std::shared_ptr<Socket>> sendSocksForOnRC,
        std::vector<std::shared_ptr<Socket>> recvSocksForOnRC)
    {
        u64 dealerIdx = 0;

        auto dealer = SepPSIDealer(
            sendSocksForOffZS[dealerIdx],
            recvSocksForOffZS[dealerIdx],
            recvSocksForOffRC,
            sendSocksForOnZS[dealerIdx],
            recvSocksForOnZS[dealerIdx],
            recvSocksForOnRC,
            dealerIdx,
            nParties,
            setSize);
        std::map<u64, SepPSIParty> parties;

        // mParties[mDealerIdx] is null.

        for (u64 i = 0; i < nParties; i++)
        {
            if (i == dealerIdx)
            {
                continue;
            }

            parties.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(i),
                std::forward_as_tuple(
                    sendSocksForOffZS[i],
                    recvSocksForOffZS[i],
                    sendSocksForOffRC[i],
                    sendSocksForOnZS[i],
                    recvSocksForOnZS[i],
                    sendSocksForOnRC[i],
                    i,
                    dealerIdx,
                    nParties,
                    setSize));
        }

        return SepPSI(std::move(dealer), std::move(parties), nParties, dealerIdx);
    }

    SepPSI SepPSI::initWithLocalAsyncSocket(u64 nParties, u64 setSize)
    {
        std::cout << "create LocalAsyncSocket" << std::endl;

        auto [sendSocksForOffZS_raw, recvSocksForOffZS_raw] = sockets::createLASocketsForZS(nParties);
        auto [sendSocksForOffRC_raw, recvSocksForOffRC_raw] = sockets::createLASocketsForRC(nParties);

        auto [sendSocksForOnZS_raw, recvSocksForOnZS_raw] = sockets::createLASocketsForZS(nParties);
        auto [sendSocksForOnRC_raw, recvSocksForOnRC_raw] = sockets::createLASocketsForRC(nParties);

        return init(
            nParties, setSize,
            toSocketVecVec<co::LocalAsyncSocket>(sendSocksForOffZS_raw), toSocketVecVec<co::LocalAsyncSocket>(recvSocksForOffZS_raw), toSocketVec<co::LocalAsyncSocket>(sendSocksForOffRC_raw), toSocketVec<co::LocalAsyncSocket>(recvSocksForOffRC_raw),
            toSocketVecVec<co::LocalAsyncSocket>(sendSocksForOnZS_raw), toSocketVecVec<co::LocalAsyncSocket>(recvSocksForOnZS_raw), toSocketVec<co::LocalAsyncSocket>(sendSocksForOnRC_raw), toSocketVec<co::LocalAsyncSocket>(recvSocksForOnRC_raw));
    }

    /*
    SepPSI SepPSI::initWithAsioSocket(u64 nParties, u64 setSize)
    {
        std::cout << "create AsioSocket" << std::endl;

        auto dealerIdx = 0;

        auto [sendSocksForOffZS_raw, recvSocksForOffZS_raw] = sockets::createAsioSocketsForZS(nParties, "localhost", 10000);
        auto [sendSocksForOffRC_raw, recvSocksForOffRC_raw] = sockets::createAsioSocketsForRC(nParties, "localhost", 20000, dealerIdx);

        auto [sendSocksForOnZS_raw, recvSocksForOnZS_raw] = sockets::createAsioSocketsForZS(nParties, "localhost", 15000);
        auto [sendSocksForOnRC_raw, recvSocksForOnRC_raw] = sockets::createAsioSocketsForRC(nParties, "localhost", 25000, dealerIdx);

        return init(
            nParties, setSize,
            toSocketVecVec<co::AsioSocket>(sendSocksForOffZS_raw), toSocketVecVec<co::AsioSocket>(recvSocksForOffZS_raw), toSocketVec<co::AsioSocket>(sendSocksForOffRC_raw), toSocketVec<co::AsioSocket>(recvSocksForOffRC_raw),
            toSocketVecVec<co::AsioSocket>(sendSocksForOnZS_raw), toSocketVecVec<co::AsioSocket>(recvSocksForOnZS_raw), toSocketVec<co::AsioSocket>(sendSocksForOnRC_raw), toSocketVec<co::AsioSocket>(recvSocksForOnRC_raw)
        );
    }
    */

    task<> SepPSI::offline(std::vector<std::shared_ptr<oc::Timer>> timers)
    {
        MC_BEGIN(task<>, this, taskIdx = u64(0), tasks = std::vector<eager_task<>>(mNParties), timers);

        for (u64 i = 0; i < mNParties; i++)
        {
            if (i == mDealerIdx)
            {
                mDealer.setTimer(*timers[i]);
                tasks[i] = mDealer.offline() | macoro::make_eager();
            }
            else
            {
                mParties.at(i).setTimer(*timers[i]);
                tasks[i] = mParties.at(i).offline() | macoro::make_eager();
            }
        }

        for (taskIdx = 0; taskIdx < mNParties; taskIdx++)
        {
            MC_AWAIT(tasks[taskIdx]);
        }

        MC_END();
    }

    task<std::vector<block>> SepPSI::online(std::vector<std::vector<block>> items, std::vector<std::shared_ptr<oc::Timer>> timers)
    {
        MC_BEGIN(task<std::vector<block>>, items, this,
                 taskIdx = u64(0),
                 tasks = std::vector<eager_task<>>(mNParties),
                 dealerTask = eager_task<std::vector<block>>(),
                 intersection = std::vector<block>(),
                 timers);

        for (u64 i = 0; i < mNParties; i++)
        {
            if (i == mDealerIdx)
            {
                mDealer.setTimer(*timers[i]);
                dealerTask = mDealer.online(items[i]) | macoro::make_eager();
            }
            else
            {
                mParties.at(i).setTimer(*timers[i]);
                tasks[i] = mParties.at(i).online(items[i]) | macoro::make_eager();
            }
        }

        for (taskIdx = 0; taskIdx < mNParties; taskIdx++)
        {
            if (taskIdx == mDealerIdx)
            {
                MC_AWAIT_SET(intersection, dealerTask);
            }
            else
            {
                MC_AWAIT(tasks[taskIdx]);
            }
        }

        MC_RETURN(intersection);

        MC_END();
    }

    void timer_summarization(std::vector<std::shared_ptr<oc::Timer>> timers)
    {
        for (auto timer : timers)
        {
            std::cout << *timer << std::endl;
        }
    }

    std::vector<block> SepPSI::run(SepPSI psi, u64 nParties, std::vector<std::vector<block>> items)
    {
        std::vector<std::shared_ptr<oc::Timer>> offlineTimers(nParties);
        std::vector<std::shared_ptr<oc::Timer>> onlineTimers(nParties);

        for (u64 i = 0; i < nParties; i++)
        {
            offlineTimers[i] = std::make_shared<oc::Timer>();
            onlineTimers[i] = std::make_shared<oc::Timer>();
        }

        // offline

        auto offlineTask = psi.offline(offlineTimers) | make_eager();

        sync_wait(when_all_ready(std::move(offlineTask)));

        // online

        auto onlineTask = psi.online(items, onlineTimers) | make_eager();

        auto v = sync_wait(std::move(onlineTask) | wrap());

        if (v.has_error())
        {
            try
            {
                v.error();
            }
            catch (std::exception &e)
            {
                std::cout << "error!" << e.what() << std::endl;
                return std::vector<block>();
            }
        }

        auto res = v.value();

        /*
        std::cout << "result:" << std::endl;
        for (auto item = res.begin(); item != res.end(); item++)
        {
            std::cout << *item << std::endl;
        }
        */

        std::cout << "=== Time Summarization ===\noffline:" << std::endl;

        timer_summarization(offlineTimers);

        std::cout << "online:" << std::endl;

        timer_summarization(onlineTimers);

        return res;
    }

    std::vector<block> SepPSI::runWithLocalAsyncSocket(u64 nParties, std::vector<std::vector<block>> items)
    {
        // init
        SepPSI psi = initWithLocalAsyncSocket(nParties, items[0].size());
        return run(std::move(psi), nParties, items);
    }

    /*
    std::vector<block> SepPSI::runWithAsioSocket(u64 nParties, std::vector<std::vector<block>> items)
    {
        // init
        SepPSI psi = initWithAsioSocket(nParties, items[0].size());
        return run(std::move(psi), nParties, items);
    }
    */

    void sep_psi_3party_test(int argc, char **argv)
    {
        std::cout << "sep_psi_3party_test by class" << std::endl;

        std::vector<std::vector<block>> items{
            toBlockVec(std::vector<int>{1, 2, 3, 4}),
            toBlockVec(std::vector<int>{2, 4, 3, 6}),
            toBlockVec(std::vector<int>{4, 5, 3, 6})};

        SepPSI::runWithLocalAsyncSocket(3, items);
    }
}