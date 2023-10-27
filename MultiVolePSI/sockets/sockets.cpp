#include "sockets.h"

namespace sockets
{
    std::tuple<
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>,
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>>
    createLASocketsForZS(u64 n)
    {
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>> sendSocks(n);
        std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            sendSocks[i] = std::vector<std::shared_ptr<LocalAsyncSocket>>(n);
            recvSocks[i] = std::vector<std::shared_ptr<LocalAsyncSocket>>(n);
        }

        for (auto i = 0; i < n; i++)
        {
            for (auto j = 0; j < n; j++)
            {
                auto p = LocalAsyncSocket::makePair();
                sendSocks[i][j] = std::make_shared<LocalAsyncSocket>(p[0]);
                recvSocks[j][i] = std::make_shared<LocalAsyncSocket>(p[1]);
            }
        }

        return std::tuple<
            std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>,
            std::vector<std::vector<std::shared_ptr<LocalAsyncSocket>>>>(sendSocks, recvSocks);
    }

    std::tuple<
        std::vector<std::shared_ptr<LocalAsyncSocket>>,
        std::vector<std::shared_ptr<LocalAsyncSocket>>>
    createLASocketsForRC(u64 n)
    {
        std::vector<std::shared_ptr<LocalAsyncSocket>> sendSocks(n);
        std::vector<std::shared_ptr<LocalAsyncSocket>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            auto p = LocalAsyncSocket::makePair();
            sendSocks[i] = std::make_shared<LocalAsyncSocket>(p[0]);
            recvSocks[i] = std::make_shared<LocalAsyncSocket>(p[1]);
        }

        return std::tuple<
            std::vector<std::shared_ptr<LocalAsyncSocket>>,
            std::vector<std::shared_ptr<LocalAsyncSocket>>>(sendSocks, recvSocks);
    }

    /*
    task<std::tuple<
        std::vector<std::shared_ptr<AsioSocket>>,
        std::vector<std::shared_ptr<AsioSocket>>>>
    createAsioSocketsForZS(
        u64 nParties, u64 myIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 10000
    )
    {
        MC_BEGIN(
            task<std::tuple<std::vector<std::shared_ptr<AsioSocket>>, std::vector<std::shared_ptr<AsioSocket>>>>,
            myIdx,
            nParties,
            host,
            portStart,
            &io_context,
            tmpSock = Socket(),
            port = u64(0),
            sendSocksForZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
            recvSocksForZS = std::vector<std::shared_ptr<AsioSocket>>(nParties),
            sockIdx = u64(0),
            i = int(0)
        );

        for (sockIdx = nParties - 1; sockIdx > myIdx; --sockIdx)
        {
            port = portStart + (myIdx * nParties + sockIdx);
            std::cout << "connect to " << host << ":" << port << std::endl;
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForZS[sockIdx] = std::make_shared<AsioSocket>(tmpSock);
        }

        for (sockIdx = 0; sockIdx < myIdx; ++sockIdx)
        {
            port = portStart + (sockIdx * nParties + myIdx);
            std::cout << "accept at " << host << ":" << port << std::endl;
            MC_AWAIT_SET(tmpSock, co::AsioAcceptor(host + ":" + std::to_string(port), io_context));
            recvSocksForZS[sockIdx] = std::make_shared<AsioSocket>(tmpSock);
        }

        for (i = int(myIdx) - 1; i >= 0; --i)
        {
            sockIdx = u64(i);
            port = portStart + (myIdx * nParties + sockIdx);
            std::cout << "connect to " << host << ":" << port << std::endl;
            MC_AWAIT_SET(tmpSock, co::AsioConnect(host + ":" + std::to_string(port), io_context));
            sendSocksForZS[sockIdx] = std::make_shared<AsioSocket>(tmpSock);
        }

        MC_RETURN(std::make_tuple(sendSocksForZS, recvSocksForZS));

        MC_END();
    }

    task<std::shared_ptr<co::AsioSocket>> createAsioSocketForRC_child(
        u64 nParties, u64 myIdx, u64 dealerIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 20000
    )
    {

    }

    task<std::vector<std::shared_ptr<co::AsioSocket>>> createAsioSocketsForRC_dealer(
        u64 nParties, u64 myIdx,
        boost::asio::io_context& io_context,
        std::string host = "localhost", int portStart = 20000
    );
    */

    /*
    std::tuple<
        std::vector<std::vector<std::shared_ptr<co::AsioSocket>>>,
        std::vector<std::vector<std::shared_ptr<co::AsioSocket>>>>
    createAsioSocketsForZS(u64 n, std::string host, int portStart)
    {
        std::vector<std::vector<std::shared_ptr<co::AsioSocket>>> sendSocks(n);
        std::vector<std::vector<std::shared_ptr<co::AsioSocket>>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            sendSocks[i] = std::vector<std::shared_ptr<co::AsioSocket>>(n);
            recvSocks[i] = std::vector<std::shared_ptr<co::AsioSocket>>(n);
        }

        std::cout << "beep" << std::endl;

        for (auto i = 0; i < n; i++)
        {
            for (auto j = 0; j < n; j++)
            {
                if (i == j)
                {
                    continue;
                }

                auto port = portStart + i * n + j;
                // auto s = co::asioConnect(host + ":" + std::to_string(port), false);
                auto r = co::asioConnect(host + ":" + std::to_string(port), true);

                std::cout << host + ":" + std::to_string(port) << std::endl;

                // sendSocks[i][j] = std::make_shared<co::AsioSocket>(s);
                recvSocks[j][i] = std::make_shared<co::AsioSocket>(r);
            }
        }

        for (auto i = 0; i < n; i++)
        {
            for (auto j = 0; j < n; j++)
            {
                if (i == j)
                {
                    continue;
                }

                auto port = portStart + i * n + j;
                auto s = co::asioConnect(host + ":" + std::to_string(port), false);
                // auto r = co::asioConnect(host + ":" + std::to_string(port), true);

                std::cout << host + ":" + std::to_string(port) << std::endl;

                sendSocks[i][j] = std::make_shared<co::AsioSocket>(s);
                // recvSocks[j][i] = std::make_shared<co::AsioSocket>(r);
            }
        }

        return std::tuple<
            std::vector<std::vector<std::shared_ptr<co::AsioSocket>>>,
            std::vector<std::vector<std::shared_ptr<co::AsioSocket>>>>(sendSocks, recvSocks);
    }

    std::tuple<
        std::vector<std::shared_ptr<co::AsioSocket>>,
        std::vector<std::shared_ptr<co::AsioSocket>>>
    createAsioSocketsForRC(u64 n, std::string host, int portStart, u64 dealerIdx)
    {
        std::vector<std::shared_ptr<co::AsioSocket>> sendSocks(n);
        std::vector<std::shared_ptr<co::AsioSocket>> recvSocks(n);
        for (auto i = 0; i < n; i++)
        {
            if (i == dealerIdx)
            {
                continue;
            }

            auto port = portStart + i;
            // auto s = co::asioConnect(host + ":" + std::to_string(port), false);
            auto r = co::asioConnect(host + ":" + std::to_string(port), true);
            // sendSocks[i] = std::make_shared<co::AsioSocket>(s);
            recvSocks[i] = std::make_shared<co::AsioSocket>(r);
        }

        for (auto i = 0; i < n; i++)
        {
            if (i == dealerIdx)
            {
                continue;
            }

            auto port = portStart + i;
            auto s = co::asioConnect(host + ":" + std::to_string(port), false);
            // auto r = co::asioConnect(host + ":" + std::to_string(port), true);
            sendSocks[i] = std::make_shared<co::AsioSocket>(s);
            // recvSocks[i] = std::make_shared<co::AsioSocket>(r);
        }


        return std::tuple<
            std::vector<std::shared_ptr<co::AsioSocket>>,
            std::vector<std::shared_ptr<co::AsioSocket>>>(sendSocks, recvSocks);
    }
    */

    IOCWork::IOCWork() : mWork(std::in_place, mIOContext)
    {
    }
}