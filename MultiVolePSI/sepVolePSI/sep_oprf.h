#pragma once

#include "Defines.h"
#include "volePSI/Paxos.h"
#include "libOTe/Vole/Silent/SilentVoleSender.h"
#include "libOTe/Vole/Silent/SilentVoleReceiver.h"
#include "utils.h"

namespace sepVolePSI
{

    class SepOprfSender : public oc::TimerAdapter
    {
    public:
        oc::SilentVoleSender mVoleSender;
        span<block> mB;
        block mD;
        volePSI::Baxos mPaxos;
        bool mMalicious = false;
        block mW;
        u64 mBinSize = 1 << 14;
        u64 mSsp = 40;
        bool mDebug = false;

        bool mPrepared = false;

        void setMultType(oc::MultType type) { mVoleSender.mMultType = type; };

        Proto offline_send(u64 n, PRNG &prng, Socket &chl, u64 mNumThreads = 0, bool reducedRounds = false);

        Proto online_send(u64 n, PRNG &prng, Socket &chl, u64 mNumThreads = 0);

        block eval(block v);

        void eval(span<const block> val, span<block> output, u64 mNumThreads = 0);

        Proto genVole(PRNG &prng, Socket &chl, bool reducedRounds);
    };

    class SepOprfReceiver : public oc::TimerAdapter
    {

    public:
        bool mMalicious = false;
        oc::SilentVoleReceiver mVoleRecver;
        u64 mBinSize = 1 << 14;
        u64 mSsp = 40;
        bool mDebug = false;

        bool mPrepared = false;

        void setMultType(oc::MultType type) { mVoleRecver.mMultType = type; };

        Proto offline_receive(u64 setSize, PRNG &prng, Socket &chl, u64 mNumThreads = 0, bool reducedRounds = false);

        Proto online_receive(span<const block> values, span<block> outputs, PRNG &prng, Socket &chl, u64 mNumThreads = 0);

        Proto genVole(u64 n, PRNG &prng, Socket &chl, bool reducedRounds);
    };
}