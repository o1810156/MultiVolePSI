#include "sep_psi_pg.h"
#include "utils.h"

namespace sepVolePSI_PG {
    oc::MatrixView<u8> span2matrix(span<block> vec)
    {
        return oc::MatrixView<u8>((u8 *)vec.data(), vec.size(), 16);
    }

    std::vector<block> matrix2vec(oc::MatrixView<u8> m)
    {
        return std::vector<block>((block *)m.begin(), (block *)m.end());
    }

    task<> dummy_task()
    {
        MC_BEGIN(task<>);
        MC_END();
    }

    task<> offlinePhaseForZS(
        u64 myIdx,
        u64 nParties,
        u64 setSize,
        std::vector<std::shared_ptr<Socket>> sendSocks,
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfs,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfs,
        PRNG &prng)
    {
        MC_BEGIN(task<>, myIdx, nParties, setSize, sendSocks, recvSocks, sendOpprfs, recvOpprfs, &prng,
                numThreads = u64(1),
                sendTasks = std::vector<eager_task<>>(nParties),
                recvTasks = std::vector<eager_task<>>(nParties),
                idxTask = u64(0));

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

        MC_END();
    }

    task<> offlinePhaseForRC_child(
        u64 setSize,
        Socket &sendSock,
        std::shared_ptr<SepOpprfSender> sendOpprf,
        PRNG &prng)
    {
        MC_BEGIN(task<>, setSize, &sendSock, sendOpprf, &prng,
                numThreads = u64(1));

        MC_AWAIT(sendOpprf->offline_send(setSize, prng, numThreads, sendSock));

        MC_END();
    }

    task<> offlinePhaseForRC_dealer(
        u64 myIdx,
        u64 setSize,
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfs,
        PRNG &prng)
    {
        MC_BEGIN(task<>, myIdx, setSize, recvSocks, recvOpprfs, &prng,
                numThreads = u64(1),
                recvTasks = std::vector<eager_task<>>(recvSocks.size()),
                idxTask = u64(0));

        for (u64 i = 0; i < recvSocks.size(); i++)
        {
            if (i == myIdx)
            {
                continue;
            }

            recvTasks[i] = recvOpprfs[i]->offline_receive(setSize, prng, numThreads, *recvSocks[i]) | macoro::make_eager();
        }

        for (idxTask = 0; idxTask < recvSocks.size(); idxTask++)
        {
            if (idxTask == myIdx)
            {
                continue;
            }

            MC_AWAIT(recvTasks[idxTask]);
        }

        MC_END();
    }

    task<std::vector<block>> conditional_zero_sharing(
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

        MC_RETURN(result);

        MC_END();
    }

    task<std::vector<block>> conditional_reconstruction_dealer(
        u64 myIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<block> zeroShares,
        std::vector<std::shared_ptr<Socket>> recvSocks,
        std::vector<std::shared_ptr<SepOpprfReceiver>> opprfReceivers,
        PRNG &prng)
    {
        MC_BEGIN(
            task<std::vector<block>>,
            myIdx,
            setSize = u64(mSet.size()),
            mSet,
            nParties,
            zeroShares,
            recvSocks,
            opprfReceivers,
            &prng,
            numThreads = u64(1),
            recvShares = std::vector<std::vector<block>>(nParties),
            recvTasks = std::vector<eager_task<>>(nParties),
            idxTask = u64(0),
            intersection = std::vector<block>(0));

        for (u64 idxP = 0; idxP < nParties; ++idxP)
        {
            recvShares[idxP].resize(setSize);
        }

        for (u64 idxP = 0; idxP < nParties; idxP++)
        {
            if (idxP == myIdx)
            {
                // recvTasks[idxP] = dummy_task() | macoro::make_eager();
                continue;
            }

            recvTasks[idxP] = opprfReceivers[idxP]->online_receive(setSize, mSet, span2matrix(recvShares[idxP]), prng, numThreads, *recvSocks[idxP]) | macoro::make_eager();
        }

        for (idxTask = 0; idxTask < nParties; idxTask++)
        {
            if (idxTask == myIdx)
            {
                continue;
            }

            MC_AWAIT(recvTasks[idxTask]);
        }

        for (u64 i = 0; i < setSize; ++i)
        {
            block sum = ZeroBlock;

            for (u64 idxP = 0; idxP < nParties; ++idxP)
            {
                if (idxP == myIdx)
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

        MC_RETURN(intersection);

        MC_END();
    }

    task<> conditional_reconstruction_child(
        std::vector<block> mSet,
        std::vector<block> zeroShares,
        Socket &sendSock,
        std::shared_ptr<SepOpprfSender> opprfSender,
        PRNG &prng)
    {
        MC_BEGIN(task<>,
                setSize = u64(mSet.size()),
                mSet,
                zeroShares,
                &sendSock,
                opprfSender,
                &prng,
                numThreads = u64(1));

        MC_AWAIT(opprfSender->online_send(setSize, mSet, span2matrix(zeroShares), prng, numThreads, sendSock));

        MC_END();
    }

    task<> party_offline(
        u64 myIdx,
        u64 dealerIdx,
        u64 setSize,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::shared_ptr<Socket> sendSockForRC,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForZS,
        std::shared_ptr<SepOpprfSender> sendOpprfForRC,
        PRNG &prng)
    {
        MC_BEGIN(task<>,
                myIdx,
                dealerIdx,
                setSize,
                nParties,
                sendSocksForZS,
                recvSocksForZS,
                sendSockForRC,
                sendOpprfsForZS,
                recvOpprfsForZS,
                sendOpprfForRC,
                &prng);
        print_debug(myIdx, "party_offline", "start");

        MC_AWAIT(offlinePhaseForZS(myIdx, nParties, setSize, sendSocksForZS, recvSocksForZS, sendOpprfsForZS, recvOpprfsForZS, prng));

        print_debug(myIdx, "party", "offlinePhaseForZS done");

        MC_AWAIT(offlinePhaseForRC_child(setSize, *sendSockForRC, sendOpprfForRC, prng));

        print_debug(myIdx, "party", "offlinePhaseForRC_child done");

        MC_END();
    }

    task<> party_online(
        u64 myIdx,
        u64 dealerIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::shared_ptr<Socket> sendSockForRC,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForZS,
        std::shared_ptr<SepOpprfSender> sendOpprfForRC,
        PRNG &prng)
    {
        MC_BEGIN(task<>,
                myIdx,
                dealerIdx,
                mSet,
                nParties,
                sendSocksForZS,
                recvSocksForZS,
                sendSockForRC,
                sendOpprfsForZS,
                recvOpprfsForZS,
                sendOpprfForRC,
                zeroShares = std::vector<block>(nParties),
                &prng);

        print_debug(myIdx, "party_online", "start");

        MC_AWAIT_SET(zeroShares, conditional_zero_sharing(myIdx, mSet, nParties, sendSocksForZS, recvSocksForZS, sendOpprfsForZS, recvOpprfsForZS, prng));

        print_debug(myIdx, "party", "conditional_zero_sharing done");

        MC_AWAIT(conditional_reconstruction_child(mSet, zeroShares, *sendSockForRC, sendOpprfForRC, prng));

        print_debug(myIdx, "party", "conditional_reconstruction_child done");

        MC_END();
    }

    task<> party_offline_dealer(
        u64 myIdx,
        u64 setSize,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForRC,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForRC,
        PRNG &prng)
    {
        MC_BEGIN(task<>,
                myIdx,
                setSize,
                nParties,
                sendSocksForZS,
                recvSocksForZS,
                recvSocksForRC,
                sendOpprfsForZS,
                recvOpprfsForZS,
                recvOpprfsForRC,
                &prng);

        print_debug(myIdx, "party_offline_dealer", "start");

        MC_AWAIT(offlinePhaseForZS(myIdx, nParties, setSize, sendSocksForZS, recvSocksForZS, sendOpprfsForZS, recvOpprfsForZS, prng));

        print_debug(myIdx, "party_dealer", "offlinePhaseForZS done");

        MC_AWAIT(offlinePhaseForRC_dealer(myIdx, setSize, recvSocksForRC, recvOpprfsForRC, prng));

        print_debug(myIdx, "party_dealer", "offlinePhaseForRC_dealer done");

        MC_END();
    }

    task<std::vector<block>> party_online_dealer(
        u64 myIdx,
        std::vector<block> mSet,
        u64 nParties,
        std::vector<std::shared_ptr<Socket>> sendSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForZS,
        std::vector<std::shared_ptr<Socket>> recvSocksForRC,
        std::vector<std::shared_ptr<SepOpprfSender>> sendOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForZS,
        std::vector<std::shared_ptr<SepOpprfReceiver>> recvOpprfsForRC,
        PRNG &prng)
    {
        MC_BEGIN(task<std::vector<block>>,
                myIdx,
                mSet,
                nParties,
                sendSocksForZS,
                recvSocksForZS,
                recvSocksForRC,
                sendOpprfsForZS,
                recvOpprfsForZS,
                recvOpprfsForRC,
                zeroShares = std::vector<block>(nParties),
                &prng,
                intersection = std::vector<block>(0));
        print_debug(myIdx, "party_online_dealer", "start");

        MC_AWAIT_SET(zeroShares, conditional_zero_sharing(myIdx, mSet, nParties, sendSocksForZS, recvSocksForZS, sendOpprfsForZS, recvOpprfsForZS, prng));

        print_debug(myIdx, "party_dealer", "conditional_zero_sharing done");

        MC_AWAIT_SET(intersection, conditional_reconstruction_dealer(myIdx, mSet, nParties, zeroShares, recvSocksForRC, recvOpprfsForRC, prng));

        print_debug(myIdx, "party_dealer", "conditional_reconstruction_dealer done");

        MC_RETURN(intersection);

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

    std::vector<std::shared_ptr<Socket>> toSocketVec(std::vector<std::shared_ptr<co::LocalAsyncSocket>> vec)
    {
        std::vector<std::shared_ptr<Socket>> ret(vec.size());
        for (u64 i = 0; i < vec.size(); ++i)
        {
            ret[i] = vec[i];
        }
        return ret;
    }

    std::tuple<
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>,
        std::vector<std::vector<std::shared_ptr<co::LocalAsyncSocket>>>>
    createSocketsForZS(int n)
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
    createSocketsForRC(int n)
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

    std::tuple<
        std::vector<std::vector<std::shared_ptr<SepOpprfSender>>>,
        std::vector<std::vector<std::shared_ptr<SepOpprfReceiver>>>>
    createOpprfPairsForZS(u64 nParties)
    {
        auto sendOpprfs = std::vector<std::vector<std::shared_ptr<SepOpprfSender>>>(nParties);
        auto recvOpprfs = std::vector<std::vector<std::shared_ptr<SepOpprfReceiver>>>(nParties);

        for (u64 idxP = 0; idxP < nParties; ++idxP)
        {
            sendOpprfs[idxP].resize(nParties);
            recvOpprfs[idxP].resize(nParties);
        }

        for (u64 idxP = 0; idxP < nParties; ++idxP)
        {
            for (u64 idxQ = 0; idxQ < nParties; ++idxQ)
            {
                sendOpprfs[idxP][idxQ] = std::make_shared<SepOpprfSender>();
                recvOpprfs[idxP][idxQ] = std::make_shared<SepOpprfReceiver>();
            }
        }

        return std::make_tuple(sendOpprfs, recvOpprfs);
    }

    std::tuple<
        std::vector<std::shared_ptr<SepOpprfSender>>,
        std::vector<std::shared_ptr<SepOpprfReceiver>>>
    createOpprfPairsForRC(u64 nParties)
    {
        auto sendOpprfs = std::vector<std::shared_ptr<SepOpprfSender>>(nParties);
        auto recvOpprfs = std::vector<std::shared_ptr<SepOpprfReceiver>>(nParties);

        for (u64 idxP = 0; idxP < nParties; ++idxP)
        {
            sendOpprfs[idxP] = std::make_shared<SepOpprfSender>();
            recvOpprfs[idxP] = std::make_shared<SepOpprfReceiver>();
        }

        return std::make_tuple(sendOpprfs, recvOpprfs);
    }

    int sep_psi_3party_test(int argc, char **argv)
    {
        // offline

        auto setSize = 4;

        // ========================

        auto [sendSocksForOffZS, recvSocksForOffZS] = createSocketsForZS(3);

        auto p0SendForOffZS = toSocketVec(sendSocksForOffZS[0]);
        auto p0RecvForOffZS = toSocketVec(recvSocksForOffZS[0]);

        auto p1SendForOffZS = toSocketVec(sendSocksForOffZS[1]);
        auto p1RecvForOffZS = toSocketVec(recvSocksForOffZS[1]);

        auto p2SendForOffZS = toSocketVec(sendSocksForOffZS[2]);
        auto p2RecvForOffZS = toSocketVec(recvSocksForOffZS[2]);

        auto [sendSocksForOffRC_raw, recvSocksForOffRC_raw] = createSocketsForRC(3);

        auto sendSocksForOffRC = toSocketVec(sendSocksForOffRC_raw);
        auto recvSocksForOffRC = toSocketVec(recvSocksForOffRC_raw);

        // ========================

        auto [sendSocksForOnZS, recvSocksForOnZS] = createSocketsForZS(3);

        auto p0SendForOnZS = toSocketVec(sendSocksForOnZS[0]);
        auto p0RecvForOnZS = toSocketVec(recvSocksForOnZS[0]);

        auto p1SendForOnZS = toSocketVec(sendSocksForOnZS[1]);
        auto p1RecvForOnZS = toSocketVec(recvSocksForOnZS[1]);

        auto p2SendForOnZS = toSocketVec(sendSocksForOnZS[2]);
        auto p2RecvForOnZS = toSocketVec(recvSocksForOnZS[2]);

        auto [sendSocksForOnRC_raw, recvSocksForOnRC_raw] = createSocketsForRC(3);

        auto sendSocksForOnRC = toSocketVec(sendSocksForOnRC_raw);
        auto recvSocksForOnRC = toSocketVec(recvSocksForOnRC_raw);

        // ========================

        auto [sendOpprfsForZS, recvOpprfsForZS] = createOpprfPairsForZS(3);
        auto [sendOpprfsForRC, recvOpprfsForRC] = createOpprfPairsForRC(3);

        auto prng0 = PRNG(_mm_set_epi32(4253465, 3434565, 0, 0));
        auto prng1 = PRNG(_mm_set_epi32(4253465, 3434565, 1, 1));
        auto prng2 = PRNG(_mm_set_epi32(4253465, 3434565, 2, 2));

        auto p0OffTask = party_offline_dealer(0, setSize, 3, p0SendForOffZS, p0RecvForOffZS, recvSocksForOffRC, sendOpprfsForZS[0], recvOpprfsForZS[0], recvOpprfsForRC, prng0) | make_eager();
        auto p1OffTask = party_offline(1, 0, setSize, 3, p1SendForOffZS, p1RecvForOffZS, sendSocksForOffRC[1], sendOpprfsForZS[1], recvOpprfsForZS[1], sendOpprfsForRC[1], prng1) | make_eager();
        auto p2OffTask = party_offline(2, 0, setSize, 3, p2SendForOffZS, p2RecvForOffZS, sendSocksForOffRC[2], sendOpprfsForZS[2], recvOpprfsForZS[2], sendOpprfsForRC[2], prng2) | make_eager();

        sync_wait(when_all_ready(std::move(p0OffTask), std::move(p1OffTask), std::move(p2OffTask)));

        // online

        auto p0 = toBlockVec(std::vector<int>{1, 2, 3, 4});
        auto p1 = toBlockVec(std::vector<int>{2, 4, 3, 6});
        auto p2 = toBlockVec(std::vector<int>{4, 5, 3, 6});

        auto p0OnTask = party_online_dealer(0, p0, 3, p0SendForOnZS, p0RecvForOnZS, recvSocksForOnRC, sendOpprfsForZS[0], recvOpprfsForZS[0], recvOpprfsForRC, prng0) | make_eager();
        auto p1OnTask = party_online(1, 0, p1, 3, p1SendForOnZS, p1RecvForOnZS, sendSocksForOnRC[1], sendOpprfsForZS[1], recvOpprfsForZS[1], sendOpprfsForRC[1], prng1) | make_eager();
        auto p2OnTask = party_online(2, 0, p2, 3, p2SendForOnZS, p2RecvForOnZS, sendSocksForOnRC[2], sendOpprfsForZS[2], recvOpprfsForZS[2], sendOpprfsForRC[2], prng2) | make_eager();

        sync_wait(when_all_ready(std::move(p1OnTask), std::move(p2OnTask)));

        auto v = sync_wait(std::move(p0OnTask) | wrap());

        if (v.has_error())
        {
            try
            {
                v.error();
            }
            catch (std::exception &e)
            {
                std::cout << "error!" << e.what() << std::endl;
                return 0;
            }
        }

        std::cout << "result:" << std::endl;

        auto res = v.value();

        for (auto item = res.begin(); item != res.end(); item++)
        {
            std::cout << "[" << item->mData[0] << ", " << item->mData[1] << "]" << std::endl;
        }

        return 0;
    }
}