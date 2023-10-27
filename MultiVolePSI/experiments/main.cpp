#include <iostream>
#include "sepVolePSI/sep_psi.h"
#include "monoVolePSI/mono_psi.h"
#include "exp.h"
#include "cryptoTools/Common/CLP.h"

using namespace experiments;

// test_with_print(3, 2, 5, 10);

/*
std::cout << "Party size: " << nParties << std::endl;
std::cout << "Common size: " << commonSize << std::endl;
std::cout << "Set size: " << setSize << std::endl;
*/

int pre_main(int argc, char **argv)
{
    oc::CLP cmd(argc, argv);

    // -n 5
    auto nParties = cmd.getOr<u64>("n", 5);

    // -ecom 10
    auto e_commonSize = cmd.getOr<u64>("ecom", 10);

    // -com 1024
    auto commonSize = cmd.getOr<u64>("com", u64(1) << e_commonSize);

    // -esize 20
    auto e_setSize = cmd.getOr<u64>("esize", 20);

    // -size 1048576
    auto setSize = cmd.getOr<u64>("size", u64(1) << e_setSize);

    // -mono
    auto mono = cmd.isSet("mono");
    auto sep = cmd.isSet("sep");

    if ((mono && sep) || (!mono && !sep))
    {
        std::cout << "Warning!: mono and sep are exclusive. mono is going to used" << std::endl;
        sep = false;
        mono = true;
    }

    // -sock asio
    auto StrSockType = cmd.getOr<std::string>("sock", "localasync");

    SockType sockType;
    if (StrSockType == "localasync")
    {
        sockType = SockType::LocalAsync;
    }
    else if (StrSockType == "asio")
    {
        sockType = SockType::Asio;
    }
    else
    {
        std::cout << "Warning!: sock type is invalid. localasync is going to used" << std::endl;
        sockType = SockType::LocalAsync;
    }

    if (mono)
    {
        test<monoVolePSI::MonoPSI>(nParties, commonSize, setSize, 10, sockType);
    }
    else
    {
        test<sepVolePSI::SepPSI>(nParties, commonSize, setSize, 10, sockType);
    }

    return 0;
}

int main(int argc, char **argv)
{
    oc::CLP cmd(argc, argv);

    // -n 5
    auto nParties = cmd.getOr<u64>("n", 5);

    // -ecom 10
    auto e_commonSize = cmd.getOr<u64>("ecom", 10);

    // -com 1024
    auto commonSize = cmd.getOr<u64>("com", u64(1) << e_commonSize);

    // -esize 20
    auto e_setSize = cmd.getOr<u64>("esize", 20);

    // -size 1048576
    auto setSize = cmd.getOr<u64>("size", u64(1) << e_setSize);

    auto sep = cmd.isSet("sep");

    auto verbose = cmd.isSet("v");

    if (sep)
    {
        test_with_asio<
            sepVolePSI::SepPSIParty,
            sepVolePSI::SepPSIDealer,
            sepVolePSI::SepPSIDealerStandaloneRes>(nParties, commonSize, setSize, 10, verbose);
    }
    else
    {
        test_with_asio<
            monoVolePSI::MonoPSIParty,
            monoVolePSI::MonoPSIDealer,
            monoVolePSI::MonoPSIDealerStandaloneRes>(nParties, commonSize, setSize, 10, verbose);
    }
}