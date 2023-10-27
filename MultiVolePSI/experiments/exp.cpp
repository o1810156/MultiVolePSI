#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <optional>
#include <future>
#include "boost/asio.hpp"
#include "exp.h"
#include "sepVolePSI/sep_psi.h"
#include "monoVolePSI/mono_psi.h"

namespace co = coproto;
using co::Socket, co::task;

namespace ma = macoro;
using ma::eager_task, ma::make_eager, ma::sync_wait, ma::when_all_ready, ma::wrap;

namespace experiments
{
    std::unordered_set<block> randomSet(u64 size, u64 seed)
    {
        std::unordered_set<block> set;
        oc::PRNG prng(_mm_set_epi32(3425431, 2543215, seed, seed));

        while (set.size() < size)
        {
            set.insert(prng.get<block>());
        }

        return set;
    }

    std::unordered_set<block> randomSetWithCommonItems(u64 id, u64 nParties, u64 size, u64 seed, std::unordered_set<block> commonItems)
    {
        std::unordered_set<block> set(commonItems.begin(), commonItems.end());
        oc::PRNG prng(_mm_set_epi32(3425431, 2543215, seed, seed));

        u64 j = 0;

        while (set.size() < size)
        {
            if (j < nParties && id != j++)
            {
                set.insert(block(j - 1));
            }
            else
            {
                set.insert(prng.get<block>());
            }
        }

        return set;
    }

    std::unordered_set<block> getIntersection(std::vector<std::unordered_set<block>> sets)
    {
        if (sets.size() == 0)
        {
            return std::unordered_set<block>();
        }
        else if (sets.size() == 1)
        {
            return sets[0];
        }

        std::unordered_set<block> intersection;
        std::unordered_set<block> set1 = sets[0];

        for (auto it = set1.begin(); it != set1.end(); ++it)
        {
            bool found = true;
            for (u64 i = 1; i < sets.size(); ++i)
            {
                if (sets[i].find(*it) == sets[i].end())
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                intersection.insert(*it);
            }
        }

        return intersection;
    }

    std::tuple<
        std::vector<std::vector<block>>,
        std::unordered_set<block>>
    createPartySets(u64 nParties, u64 commonSize, u64 setSize, u64 seed)
    {
        std::vector<std::unordered_set<block>> sets(nParties);
        std::unordered_set<block> commonItems = randomSet(commonSize, seed);

        for (u64 i = 0; i < nParties; ++i)
        {
            sets[i] = randomSetWithCommonItems(i, nParties, setSize, seed + i, commonItems);
        }

        std::unordered_set<block> intersection = getIntersection(sets);
        std::vector<std::vector<block>> partySets(nParties);
        std::mt19937_64 get_rand_mt;

        for (u64 i = 0; i < nParties; ++i)
        {
            std::vector<block> set(sets[i].begin(), sets[i].end());

            // shuffle
            // https://www.albow.net/entry/shuffle-vector
            std::shuffle(set.begin(), set.end(), get_rand_mt);

            partySets[i] = set;
        }

        return std::make_tuple(partySets, intersection);
    }

    template <class PSI>
    void test_with_print(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType)
    {
        auto [items, commonItems] = experiments::createPartySets(nParties, commonSize, setSize, seed);

        std::cout << "Common items: ";
        for (auto item : commonItems)
        {
            std::cout << item << " ";
        }

        for (u64 i = 0; i < items.size(); i++)
        {
            std::cout << "Party " << i << ": ";
            for (auto item : items[i])
            {
                std::cout << item << " ";
            }
            std::cout << std::endl;
        }

        std::vector<oc::block> res;
        switch (sockType)
        {
        case SockType::LocalAsync:
            res = PSI::runWithLocalAsyncSocket(nParties, items);
            break;
        case SockType::Asio:
            // res = PSI::runWithAsioSocket(nParties, items);
            break;
        }

        std::cout << "Result: ";
        for (auto item : res)
        {
            std::cout << item << " ";
        }
        std::cout << std::endl;

        std::cout << "Check: ";
        for (auto item : res)
        {
            if (commonItems.find(item) == commonItems.end())
            {
                std::cout << "Error" << std::endl;
                return;
            }
        }
        std::cout << "OK" << std::endl;
    }

    template <class PSI>
    void test(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType)
    {
        auto [items, commonItems] = experiments::createPartySets(nParties, commonSize, setSize, seed);

        std::cout << "Common items num: " << commonItems.size() << std::endl;
        std::cout << "Party items num: " << items[0].size() << std::endl;

        oc::Timer totalTimer;
        totalTimer.setTimePoint("start");

        std::vector<oc::block> res;
        switch (sockType)
        {
        case SockType::LocalAsync:
            res = PSI::runWithLocalAsyncSocket(nParties, items);
            break;
        case SockType::Asio:
            // res = PSI::runWithAsioSocket(nParties, items);
            break;
        }

        totalTimer.setTimePoint("end");

        std::cout << "Total time: " << totalTimer << std::endl;

        std::cout << "Check: ";
        for (auto item : res)
        {
            if (commonItems.find(item) == commonItems.end())
            {
                std::cout << "Error" << std::endl;
                return;
            }
        }
        std::cout << "OK" << std::endl;
    }

    class IOCWork
    {
    public:
        IOCWork() : mWork(std::in_place, mIOContext)
        {
        }
        boost::asio::io_context mIOContext;
        std::optional<boost::asio::io_context::work> mWork;
    };

    template <class PSIParty, class PSIDealer, class DealerStandaloneRes>
    void test_with_asio(u64 nParties, u64 commonSize, u64 setSize, u64 seed, bool verbose)
    {
        auto [items, commonItems] = experiments::createPartySets(nParties, commonSize, setSize, seed);

        std::cout << "Common items num: " << commonItems.size() << std::endl;
        std::cout << "Party items num: " << items[0].size() << std::endl;

        std::vector<IOCWork> iocWorks(nParties);

        std::vector<std::future<void>> run_fs(nParties);
        std::vector<std::future<std::string>> futures(nParties);
        run_fs[0] = std::async([&]
                               { iocWorks[0].mIOContext.run(); });
        futures[0] = std::async([&]
                                { return std::string(""); });

        for (u64 i = 1; i < nParties; i++)
        {
            run_fs[i] = std::async([i, &iocWorks]
                                   { iocWorks[i].mIOContext.run(); });

            futures[i] = std::async([i, nParties, verbose, &items, &iocWorks]()
                                    {
                auto t = PSIParty::standaloneWithAsio(
                    i,
                    0,
                    nParties,
                    items[i],
                    "localhost",
                    10000,
                    20000,
                    15000,
                    25000,
                    iocWorks[i].mIOContext,
                    true,
                    verbose
                );
                auto log = sync_wait(std::move(t));

                return log; });
        }

        eager_task<DealerStandaloneRes> dealerTask = PSIDealer::standaloneWithAsio(
                                                         0,
                                                         nParties,
                                                         items[0],
                                                         "localhost",
                                                         10000,
                                                         20000,
                                                         15000,
                                                         25000,
                                                         iocWorks[0].mIOContext,
                                                         true,
                                                         verbose) |
                                                     make_eager();

        auto v = sync_wait(std::move(dealerTask) | wrap());

        if (v.has_error())
        {
            try
            {
                v.error();
            }
            catch (std::exception &e)
            {
                std::cout << "error!" << e.what() << std::endl;
                return;
            }
        }

        auto res = v.value().mIntersection;

        std::cout << "Dealer: \n"
                  << v.value().mLog << std::endl;

        for (u64 i = 0; i < nParties; i++)
        {
            iocWorks[i].mWork.reset();
            run_fs[i].get();
            auto log = futures[i].get();

            if (i != 0)
            {
                std::cout << "Party " << i << ": \n"
                          << log << std::endl;
            }
        }

        std::cout << "Check: ";
        for (auto item : res)
        {
            if (commonItems.find(item) == commonItems.end())
            {
                std::cout << "Error" << std::endl;
                return;
            }
        }
        std::cout << "OK" << std::endl;
    }

    /*
    template <class PSIParty, class PSIDealer, class DealerStandaloneRes>
    void test_with_asio(u64 nParties, u64 commonSize, u64 setSize, u64 seed, bool verbose)
    {
        auto [items, commonItems] = experiments::createPartySets(nParties, commonSize, setSize, seed);

        std::cout << "Common items num: " << commonItems.size() << std::endl;
        std::cout << "Party items num: " << items[0].size() << std::endl;

        std::vector<std::future<std::string>> futures(nParties);

        futures[0] = std::async([&]
                                { return std::string(""); });

        for (u64 i = 1; i < nParties; i++)
        {
            futures[i] = std::async([i, nParties, verbose, &items]()
                                    {
                auto t = PSIParty::standaloneWithAsio(
                    i,
                    0,
                    nParties,
                    items[i],
                    "localhost",
                    10000,
                    20000,
                    15000,
                    25000,
                    true,
                    verbose
                );
                auto log = sync_wait(std::move(t));

                return log; });
        }

        eager_task<DealerStandaloneRes> dealerTask = PSIDealer::standaloneWithAsio(
                                                         0,
                                                         nParties,
                                                         items[0],
                                                         "localhost",
                                                         10000,
                                                         20000,
                                                         15000,
                                                         25000,
                                                         true,
                                                         verbose) |
                                                     make_eager();

        auto v = sync_wait(std::move(dealerTask) | wrap());

        if (v.has_error())
        {
            try
            {
                v.error();
            }
            catch (std::exception &e)
            {
                std::cout << "error!" << e.what() << std::endl;
                return;
            }
        }

        auto res = v.value().mIntersection;

        std::cout << "Dealer: \n"
                  << v.value().mLog << std::endl;

        for (u64 i = 0; i < nParties; i++)
        {
            auto log = futures[i].get();

            if (i != 0)
            {
                std::cout << "Party " << i << ": \n"
                          << log << std::endl;
            }
        }

        std::cout << "Check: ";
        for (auto item : res)
        {
            if (commonItems.find(item) == commonItems.end())
            {
                std::cout << "Error" << std::endl;
                return;
            }
        }
        std::cout << "OK" << std::endl;
    }
    */

    template void test_with_print<monoVolePSI::MonoPSI>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);
    template void test_with_print<sepVolePSI::SepPSI>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);

    template void test<monoVolePSI::MonoPSI>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);
    template void test<sepVolePSI::SepPSI>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);

    template void test_with_asio<
        monoVolePSI::MonoPSIParty, monoVolePSI::MonoPSIDealer, monoVolePSI::MonoPSIDealerStandaloneRes>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, bool verbose);

    template void test_with_asio<
        sepVolePSI::SepPSIParty, sepVolePSI::SepPSIDealer, sepVolePSI::SepPSIDealerStandaloneRes>(u64 nParties, u64 commonSize, u64 setSize, u64 seed, bool verbose);
}