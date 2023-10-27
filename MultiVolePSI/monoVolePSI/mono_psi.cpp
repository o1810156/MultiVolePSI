#include "mono_psi.h"
#include "sockets/sockets.h"
#include <future>

namespace monoVolePSI
{
    /*
    void print_debug(u64 idx, std::string scope, std::string content)
    {
        std::cout << "[" << idx << ":" << scope << "] " << content << std::endl;
    }
    */

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
    task<std::vector<block>> conditionalZeroSharing(
        T &timer,
        u64 myIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocks,
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<volePSI::RsOpprfSender>> opprfSenders,
        std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>> opprfReceivers,
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
            // opprfSenders = std::vector<std::unique_ptr<volePSI::RsOpprfSender>>(nParties),
            // opprfReceivers = std::vector<std::unique_ptr<volePSI::RsOpprfReceiver>>(nParties),
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

            sendTasks[idxP] = opprfSenders[idxP]->send(setSize, mSet, span2matrix(sendShares[idxP]), prng, numThreads, *sendSocks[idxP]) | macoro::make_eager();
            recvTasks[idxP] = opprfReceivers[idxP]->receive(setSize, mSet, span2matrix(recvShares[idxP]), prng, numThreads, *recvSocks[idxP]) | macoro::make_eager();
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
    // forkうまくいかなかった...
    std::vector<std::shared_ptr<Socket>> vector_fork(std::vector<std::shared_ptr<Socket>> socks)
    {
        std::vector<std::shared_ptr<Socket>> ret(socks.size());
        for (u64 i = 0; i < socks.size(); ++i)
        {
            ret[i] = std::make_shared<Socket>(socks[i]->fork());
        }

        return ret;
    }
    */

    MonoPSIParty::MonoPSIParty(
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::shared_ptr<Socket> sendSockForRC,
        u64 myIdx,
        u64 dealerIdx,
        u64 nParties,
        u64 setSize)
        : mSendSocksForZeroSharing(sendSocksForZS),
          mRecvSocksForZeroSharing(recvSocksForZS),
          mSendSockForReconstruction(sendSockForRC),
          mMyIdx(myIdx),
          mDealerIdx(dealerIdx),
          mNParties(nParties),
          mSetSize(setSize),
          mPrng(PRNG(_mm_set_epi32(4253465, 3434565, myIdx, myIdx)))
    {
        mOpprfSendersForZeroSharing = std::vector<std::shared_ptr<volePSI::RsOpprfSender>>(mNParties);
        mOpprfReceiversForZeroSharing = std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>>(mNParties);
        mOpprfSenderForReconstruction = std::make_shared<volePSI::RsOpprfSender>();

        for (u64 i = 0; i < nParties; ++i)
        {
            mOpprfSendersForZeroSharing[i] = std::make_shared<volePSI::RsOpprfSender>();
            mOpprfReceiversForZeroSharing[i] = std::make_shared<volePSI::RsOpprfReceiver>();
        }
    }

    void MonoPSIParty::setTimersOfOpprfs()
    {
        for (u64 idxP = 0; idxP < mNParties; ++idxP)
        {
            mOpprfSendersForZeroSharing[idxP]->setTimer(*mTimer);
            mOpprfReceiversForZeroSharing[idxP]->setTimer(*mTimer);
        }

        mOpprfSenderForReconstruction->setTimer(*mTimer);
    }

    task<> MonoPSIParty::conditionalReconstruction(
        std::vector<block> set,
        std::vector<block> zeroShares)
    {
        MC_BEGIN(task<>,
                 this,
                 set,
                 zeroShares,
                 numThreads = u64(1));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_child_start");

        MC_AWAIT(mOpprfSenderForReconstruction->send(
            u64(set.size()), set, span2matrix(zeroShares), mPrng, numThreads,
            *mSendSockForReconstruction));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_child_end");

        MC_END();
    }

    task<> MonoPSIParty::online(std::vector<block> set)
    {
        MC_BEGIN(task<>,
                 this, set,
                 zeroShares = std::vector<block>(mNParties));

        setTimersOfOpprfs();

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_online_start");

        MC_AWAIT_SET(zeroShares, conditionalZeroSharing<MonoPSIParty>(
                                     *this,
                                     mMyIdx, set, mNParties,
                                     mSendSocksForZeroSharing,
                                     mRecvSocksForZeroSharing,
                                     mOpprfSendersForZeroSharing,
                                     mOpprfReceiversForZeroSharing,
                                     mPrng));

        MC_AWAIT(conditionalReconstruction(set, zeroShares));

        setTimePoint("[" + std::to_string(mMyIdx) + "] party_online_end");

        MC_END();
    }

    task<std::string> MonoPSIParty::standaloneWithAsio(
        u64 myIdx,
        u64 dealerIdx,
        u64 nParties,
        std::vector<block> set,
        std::string host,
        u64 basePortForZS,
        u64 basePortForRC,
        u64 basePortForOnlineZS, // dummy
        u64 basePortForOnlineRC, // dummy
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
                 basePortForZS,
                 basePortForRC,
                 basePortForOnlineZS, // dummy
                 basePortForOnlineRC, // dummy
                 &io_context,
                 useTimer,
                 verbose,
                 timer = std::shared_ptr<oc::Timer>(),
                 tmpSock = Socket(),
                 port = u64(0),
                 sendSocksForZS = std::vector<std::shared_ptr<Socket>>(nParties),
                 recvSocksForZS = std::vector<std::shared_ptr<Socket>>(nParties),
                 sendSockForRC = std::make_shared<Socket>(),
                 sockIdx = u64(0),
                 i = int(0),
                 party = std::shared_ptr<MonoPSIParty>(),

                 sendIocwsForZS = std::vector<std::shared_ptr<sockets::IOCWork>>(nParties),
                 recvIocwsForZS = std::vector<std::shared_ptr<sockets::IOCWork>>(nParties),
                 sendRunFsForZS = std::vector<std::shared_ptr<std::future<void>>>(nParties),
                 recvRunFsForZS = std::vector<std::shared_ptr<std::future<void>>>(nParties),

                 sendIocwForRC = std::shared_ptr<sockets::IOCWork>(),
                 sendRunFForRC = std::shared_ptr<std::future<void>>(),

                 oss = std::ostringstream());

        if (verbose)
        {
            oss << "myItems: " << std::endl;
            for (auto item : set)
            {
                oss << item << std::endl;
            }
        }

        for (u64 i = 0; i < nParties; i++)
        {
            sendIocwsForZS[i] = std::make_shared<sockets::IOCWork>();
            recvIocwsForZS[i] = std::make_shared<sockets::IOCWork>();
        }

        sendRunFsForZS[myIdx] = std::make_shared<std::future<void>>(std::async([&] {}));

        for (sockIdx = nParties - 1; sockIdx > myIdx; --sockIdx)
        {
            sendRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &sendIocwsForZS]
                                                                                     { sendIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), sendIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
            sendSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                recvRunFsForZS[myIdx] = std::make_shared<std::future<void>>(std::async([&] {}));
                continue;
            }

            recvRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &recvIocwsForZS]
                                                                                     { recvIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), recvIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "<-" << sockIdx << "]"
                << "accepted at " << host << ":" << port << std::endl;
            recvSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        for (i = int(myIdx) - 1; i >= 0; --i)
        {
            sockIdx = u64(i);

            sendRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &sendIocwsForZS]
                                                                                     { sendIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), sendIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
            sendSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        sendRunFForRC = std::make_shared<std::future<void>>(std::async([&]
                                                                       { sendIocwForRC->mIOContext.run(); }));

        port = basePortForRC + (myIdx * nParties + dealerIdx);
        oss << "(RC) connect to " << host << ":" << port << std::endl;
        MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), sendIocwForRC->mIOContext));
        sendSockForRC = std::make_shared<Socket>(tmpSock);

        party = std::make_shared<MonoPSIParty>(MonoPSIParty(
            sendSocksForZS,
            recvSocksForZS,
            sendSockForRC,
            myIdx,
            dealerIdx,
            nParties,
            u64(set.size())));

        timer = std::make_shared<oc::Timer>();

        if (useTimer)
        {
            timer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_start");
            party->setTimer(*timer);
        }

        MC_AWAIT(party->online(set));

        if (useTimer)
        {
            oss << "=== Time Summarization [" << myIdx << "] ===" << std::endl;
            timer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_end");
            oss << *timer << std::endl;
        }

        for (u64 sockIdx = 0; sockIdx < nParties; sockIdx++)
        {
            if (sockIdx == myIdx)
            {
                continue;
            }

            sendIocwsForZS[sockIdx]->mWork.reset();
            recvIocwsForZS[sockIdx]->mWork.reset();
        }

        sendIocwForRC->mWork.reset();

        MC_RETURN(oss.str());

        MC_END();
    }

    MonoPSIDealer::MonoPSIDealer(
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForRC,
        u64 myIdx,
        u64 nParties,
        u64 setSize)
        : mSendSocksForZeroSharing(sendSocksForZS),
          mRecvSocksForZeroSharing(recvSocksForZS),
          mRecvSocksForReconstruction(recvSocksForRC),
          mMyIdx(myIdx),
          mNParties(nParties),
          mSetSize(setSize),
          mPrng(PRNG(_mm_set_epi32(4253465, 3434565, myIdx, myIdx)))
    {
        mOpprfSendersForZeroSharing = std::vector<std::shared_ptr<volePSI::RsOpprfSender>>(nParties);
        mOpprfReceiversForZeroSharing = std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>>(nParties);
        mOpprfReceiversForReconstruction = std::vector<std::shared_ptr<volePSI::RsOpprfReceiver>>(nParties);

        for (u64 i = 0; i < nParties; ++i)
        {
            mOpprfSendersForZeroSharing[i] = std::make_shared<volePSI::RsOpprfSender>();
            mOpprfReceiversForZeroSharing[i] = std::make_shared<volePSI::RsOpprfReceiver>();
            mOpprfReceiversForReconstruction[i] = std::make_shared<volePSI::RsOpprfReceiver>();
        }
    }

    void MonoPSIDealer::setTimersOfOpprfs()
    {
        for (u64 idxP = 0; idxP < mNParties; ++idxP)
        {
            mOpprfSendersForZeroSharing[idxP]->setTimer(*mTimer);
            mOpprfReceiversForZeroSharing[idxP]->setTimer(*mTimer);
            mOpprfReceiversForReconstruction[idxP]->setTimer(*mTimer);
        }
    }

    task<std::vector<block>> MonoPSIDealer::conditionalReconstruction(
        std::vector<block> set,
        std::vector<block> zeroShares)
    {
        MC_BEGIN(
            task<std::vector<block>>,
            this,
            set,
            setSize = u64(set.size()),
            zeroShares,
            numThreads = u64(1),
            recvShares = std::vector<std::vector<block>>(mNParties),
            recvTasks = std::vector<eager_task<>>(mNParties),
            idxTask = u64(0),
            intersection = std::vector<block>(0));

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_dealer_start");

        for (u64 idxP = 0; idxP < mNParties; ++idxP)
        {
            recvShares[idxP].resize(setSize);
        }

        for (u64 idxP = 0; idxP < mNParties; idxP++)
        {
            if (idxP == mMyIdx)
            {
                // recvTasks[idxP] = dummy_task() | macoro::make_eager();
                continue;
            }

            recvTasks[idxP] = mOpprfReceiversForReconstruction[idxP]->receive(
                                  setSize, set, span2matrix(recvShares[idxP]),
                                  mPrng,
                                  numThreads,
                                  *mRecvSocksForReconstruction[idxP]) |
                              macoro::make_eager();
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

        for (u64 i = 0; i < setSize; ++i)
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
                intersection.push_back(set[i]);
            }
        }

        setTimePoint("[" + std::to_string(mMyIdx) + "] conditional_reconstruction_dealer_end");

        MC_RETURN(intersection);

        MC_END();
    }

    task<std::vector<block>> MonoPSIDealer::online(std::vector<block> set)
    {
        MC_BEGIN(task<std::vector<block>>,
                 this,
                 set,
                 zeroShares = std::vector<block>(mNParties),
                 intersection = std::vector<block>(0));

        setTimersOfOpprfs();

        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_online_start");

        MC_AWAIT_SET(zeroShares, conditionalZeroSharing<MonoPSIDealer>(
                                     *this,
                                     mMyIdx,
                                     set,
                                     mNParties,
                                     mSendSocksForZeroSharing,
                                     mRecvSocksForZeroSharing,
                                     mOpprfSendersForZeroSharing,
                                     mOpprfReceiversForZeroSharing,
                                     mPrng));

        MC_AWAIT_SET(intersection, conditionalReconstruction(set, zeroShares));

        setTimePoint("[" + std::to_string(mMyIdx) + "] dealer_online_end");

        MC_RETURN(intersection);

        MC_END();
    }

    task<MonoPSIDealerStandaloneRes> MonoPSIDealer::standaloneWithAsio(
        u64 myIdx,
        u64 nParties,
        std::vector<block> set,
        std::string host,
        u64 basePortForZS,
        u64 basePortForRC,
        u64 basePortForOnlineZS, // dummy
        u64 basePortForOnlineRC, // dummy
        boost::asio::io_context &io_context,
        bool useTimer,
        bool verbose)
    {
        MC_BEGIN(task<MonoPSIDealerStandaloneRes>,
                 myIdx,
                 nParties,
                 set,
                 host,
                 basePortForZS,
                 basePortForRC,
                 basePortForOnlineZS, // dummy
                 basePortForOnlineRC, // dummy
                 &io_context,
                 useTimer,
                 verbose,
                 timer = std::shared_ptr<oc::Timer>(),
                 tmpSock = Socket(),
                 port = u64(0),
                 sendSocksForZS = std::vector<std::shared_ptr<Socket>>(nParties),
                 recvSocksForZS = std::vector<std::shared_ptr<Socket>>(nParties),
                 recvSocksForRC = std::vector<std::shared_ptr<Socket>>(nParties),
                 sockIdx = u64(0),
                 i = int(0),
                 party = std::shared_ptr<MonoPSIDealer>(),
                 intersection = std::vector<block>(0),

                 sendIocwsForZS = std::vector<std::shared_ptr<sockets::IOCWork>>(nParties),
                 recvIocwsForZS = std::vector<std::shared_ptr<sockets::IOCWork>>(nParties),
                 sendRunFsForZS = std::vector<std::shared_ptr<std::future<void>>>(nParties),
                 recvRunFsForZS = std::vector<std::shared_ptr<std::future<void>>>(nParties),

                 sendIocwsForRC = std::vector<std::shared_ptr<sockets::IOCWork>>(nParties),
                 sendRunFsForRC = std::vector<std::shared_ptr<std::future<void>>>(nParties),

                 oss = std::ostringstream());

        if (verbose)
        {
            oss << "myItems: " << std::endl;
            for (auto item : set)
            {
                oss << item << std::endl;
            }
        }

        for (u64 i = 0; i < nParties; i++)
        {
            sendIocwsForZS[i] = std::make_shared<sockets::IOCWork>();
            recvIocwsForZS[i] = std::make_shared<sockets::IOCWork>();
            sendIocwsForRC[i] = std::make_shared<sockets::IOCWork>();
        }

        sendRunFsForZS[myIdx] = std::make_shared<std::future<void>>(std::async([&] {}));

        for (sockIdx = nParties - 1; sockIdx > myIdx; --sockIdx)
        {

            sendRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &sendIocwsForZS]
                                                                                     { sendIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), sendIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
            sendSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                recvRunFsForZS[myIdx] = std::make_shared<std::future<void>>(std::async([&] {}));
                continue;
            }

            recvRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &recvIocwsForZS]
                                                                                     { recvIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (sockIdx * nParties + myIdx);
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), recvIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "<-" << sockIdx << "]"
                << "accepted at " << host << ":" << port << std::endl;
            recvSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        for (i = int(myIdx) - 1; i >= 0; --i)
        {
            sockIdx = u64(i);

            sendRunFsForZS[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &sendIocwsForZS]
                                                                                     { sendIocwsForZS[sockIdx]->mIOContext.run(); }));

            port = basePortForZS + (myIdx * nParties + sockIdx);
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), sendIocwsForZS[sockIdx]->mIOContext));
            oss << "[" << myIdx << "->" << sockIdx << "]"
                << "connected to " << host << ":" << port << std::endl;
            sendSocksForZS[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        for (sockIdx = 0; sockIdx < nParties; ++sockIdx)
        {
            if (sockIdx == myIdx)
            {
                sendRunFsForRC[myIdx] = std::make_shared<std::future<void>>(std::async([&] {}));
                continue;
            }

            sendRunFsForRC[sockIdx] = std::make_shared<std::future<void>>(std::async([sockIdx, &sendIocwsForRC]
                                                                                     { sendIocwsForRC[sockIdx]->mIOContext.run(); }));

            port = basePortForRC + (sockIdx * nParties + myIdx);
            oss << "(RC) accept at " << host << ":" << port << std::endl;
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), sendIocwsForRC[sockIdx]->mIOContext));
            recvSocksForRC[sockIdx] = std::make_shared<Socket>(tmpSock);
        }

        party = std::make_shared<MonoPSIDealer>(MonoPSIDealer(
            sendSocksForZS,
            recvSocksForZS,
            recvSocksForRC,
            myIdx,
            nParties,
            u64(set.size())));

        timer = std::make_shared<oc::Timer>();

        if (useTimer)
        {
            timer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_start");
            party->setTimer(*timer);
        }

        MC_AWAIT_SET(intersection, party->online(set));

        if (useTimer)
        {
            oss << "=== Time Summarization [" << myIdx << "] ===" << std::endl;
            timer->setTimePoint("[" + std::to_string(myIdx) + "] standalone_with_asio_end");
            oss << *timer << std::endl;
        }

        for (u64 sockIdx = 0; sockIdx < nParties; sockIdx++)
        {
            if (sockIdx == myIdx)
            {
                continue;
            }

            sendIocwsForZS[sockIdx]->mWork.reset();
            recvIocwsForZS[sockIdx]->mWork.reset();
            sendIocwsForRC[sockIdx]->mWork.reset();
        }

        MC_RETURN(MonoPSIDealerStandaloneRes(oss.str(), intersection));

        MC_END();
    }

    MonoPSI::MonoPSI(
        MonoPSIDealer dealer,
        std::map<u64, MonoPSIParty> parties,
        u64 nParties,
        u64 dealerIdx)
        : mDealer(std::move(dealer)),
          mParties(std::move(parties)),
          mNParties(nParties),
          mDealerIdx(dealerIdx)
    {
    }

    MonoPSI MonoPSI::init(
        u64 nParties, u64 setSize,
        std::vector<std::vector<std::shared_ptr<Socket>>> sendSocksForZS,
        std::vector<std::vector<std::shared_ptr<Socket>>> recvSocksForZS,
        std::vector<std::shared_ptr<Socket>> sendSocksForRC,
        std::vector<std::shared_ptr<Socket>> recvSocksForRC)
    {
        auto dealerIdx = 0;

        auto dealer = MonoPSIDealer(
            sendSocksForZS[dealerIdx],
            recvSocksForZS[dealerIdx],
            recvSocksForRC,
            dealerIdx,
            nParties,
            setSize);
        std::map<u64, MonoPSIParty> parties;

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
                    sendSocksForZS[i],
                    recvSocksForZS[i],
                    sendSocksForRC[i],
                    i,
                    dealerIdx,
                    nParties,
                    setSize));
        }

        return MonoPSI(std::move(dealer), std::move(parties), nParties, dealerIdx);
    }

    MonoPSI MonoPSI::initWithLocalAsyncSocket(u64 nParties, u64 setSize)
    {
        auto [sendSocksForZS_raw, recvSocksForZS_raw] = sockets::createLASocketsForZS(nParties);
        auto [sendSocksForRC_raw, recvSocksForRC_raw] = sockets::createLASocketsForRC(nParties);

        return init(
            nParties, setSize,
            toSocketVecVec<co::LocalAsyncSocket>(sendSocksForZS_raw),
            toSocketVecVec<co::LocalAsyncSocket>(recvSocksForZS_raw),
            toSocketVec<co::LocalAsyncSocket>(sendSocksForRC_raw),
            toSocketVec<co::LocalAsyncSocket>(recvSocksForRC_raw));
    }

    /*
    MonoPSI MonoPSI::initWithAsioSocket(u64 nParties, u64 setSize)
    {
        auto [sendSocksForZS_raw, recvSocksForZS_raw] = sockets::createAsioSocketsForZS(nParties, "localhost", 10000);
        auto [sendSocksForRC_raw, recvSocksForRC_raw] = sockets::createAsioSocketsForRC(nParties, "localhost", 20000);

        return init(
            nParties, setSize,
            toSocketVecVec<co::AsioSocket>(sendSocksForZS_raw),
            toSocketVecVec<co::AsioSocket>(recvSocksForZS_raw),
            toSocketVec<co::AsioSocket>(sendSocksForRC_raw),
            toSocketVec<co::AsioSocket>(recvSocksForRC_raw)
        );
    }
    */

    task<std::vector<block>> MonoPSI::online(std::vector<std::vector<block>> items, std::vector<std::shared_ptr<oc::Timer>> timers)
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

    std::vector<block> MonoPSI::run(MonoPSI psi, u64 nParties, std::vector<std::vector<block>> items)
    {
        std::vector<std::shared_ptr<oc::Timer>> onlineTimers(nParties);

        for (u64 i = 0; i < nParties; i++)
        {
            onlineTimers[i] = std::make_shared<oc::Timer>();
        }

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

        std::cout << "=== Time Summarization ===" << std::endl;

        timer_summarization(onlineTimers);

        return res;
    }

    std::vector<block> MonoPSI::runWithLocalAsyncSocket(u64 nParties, std::vector<std::vector<block>> items)
    {
        // init
        MonoPSI psi = initWithLocalAsyncSocket(nParties, items[0].size());
        return run(std::move(psi), nParties, items);
    }

    /*
    std::vector<block> MonoPSI::runWithAsioSocket(u64 nParties, std::vector<std::vector<block>> items)
    {
        // init
        MonoPSI psi = initWithAsioSocket(nParties, items[0].size());
        return run(std::move(psi), nParties, items);
    }
    */

    void mono_psi_3party_test(int argc, char **argv)
    {
        std::cout << "mono_psi_3party_test by class" << std::endl;

        std::vector<std::vector<block>> items{
            toBlockVec(std::vector<int>{1, 2, 3, 4}),
            toBlockVec(std::vector<int>{2, 4, 3, 6}),
            toBlockVec(std::vector<int>{4, 5, 3, 6})};

        MonoPSI::runWithLocalAsyncSocket(3, items);
    }
}
