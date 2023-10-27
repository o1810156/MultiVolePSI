#include "cryptoTools/Common/Defines.h"
// #include "cryptoTools/Common/Log.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "coproto/coproto.h"
#include "coproto/Socket/Socket.h"
#include "volePSI/RsOpprf.h"
#include <vector>
#include <iostream>
#include <algorithm>

namespace oc = osuCrypto;
using oc::u64, oc::u32, oc::u8, oc::u8,
    oc::block,
    oc::PRNG,
    oc::ZeroBlock;

namespace co = coproto;
using co::Socket, co::task;

using macoro::eager_task;

task<> party(
    u64 myIdx,
    u64 setSize,
    std::vector<block> mSet,
    u64 nParties,
    std::vector<Socket&> sendSocks, // myIdx番目(自分宛)のも便宜上含む。つまり len(sendSocks) = nParties である
    std::vector<Socket&> recvSocks)
{
    MC_BEGIN(
        task<>,
        myIdx,
        setSize,
        mSet,
        nParties,
        sendSocks,
        recvSocks,
        psiSecParam = u64(40),
        bitSize = u64(128),
        numThreads = u64(1),
        prng = PRNG(_mm_set_epi32(4253465, 3434565, myIdx, myIdx)),
        // std::vector<std::vector<block>> sendShares(nParties), recvPayLoads(nParties); // original
        sendShares = std::vector<oc::Matrix<u8>>(nParties),
        recvShares = std::vector<oc::Matrix<u8>>(nParties),
        sendTasks = std::vector<eager_task<>>(nParties - 1),
        recvTasks = std::vector<eager_task<>>(nParties - 1)
    );

    for (u64 idxP = 0; idxP < nParties; ++idxP)
    {

        // vecotr<block> <-> Matrix<u8> の解決から!









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

    // std::transform を使いたいがわかりやすさのために for 文を使う

    for (u64 i = 0; i < nParties; ++i)
    {
        if (i == myIdx)
        {
            continue;
        }

        // recvTasks[i] = recvSocks[i].recv(recvShares[i]);
        // co::optional<macoro::eager_task<>> senderTask = sender_run(senderSocket, setSize, sendSet, sendProgramedVals) | macoro::make_eager();
        sendTasks[i] = sendSocks[i].send(setSize, mSet, sendProgramedVals, sprng, numThreads, senderSocket);
    }

    MC_END();
}

/*
    std::string name("psi");
    IOService ios(0);

    int btCount = nParties;
    std::vector<Endpoint> ep(nParties);

    for (u64 i = 0; i < myIdx; ++i)
    {
        u32 port = 1120 + i * 100 + myIdx;                         // get the same port; i=1 & pIdx=2 =>port=102
        ep[i].start(ios, "localhost", port, EpMode::Client, name); // channel bwt i and pIdx, where i is sender
    }

    for (u64 i = myIdx + 1; i < nParties; ++i)
    {
        u32 port = 1120 + myIdx * 100 + i;                         // get the same port; i=2 & pIdx=1 =>port=102
        ep[i].start(ios, "localhost", port, EpMode::Server, name); // channel bwt i and pIdx, where i is receiver
    }

    std::vector<std::vector<Channel *>> chls(nParties);

    for (u64 i = 0; i < nParties; ++i)
    {
        if (i == myIdx)
        {
            continue;
        }

        chls[i].resize(numThreads);
        for (u64 j = 0; j < numThreads; ++j)
        {
            chls[i][j] = &ep[i].addChannel("chl" + std::to_string(j), "chl" + std::to_string(j));
            // chls[i][j] = &ep[i].addChannel(name, name);
        }
    }
*/