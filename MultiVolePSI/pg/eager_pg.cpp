#include "coproto/coproto.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace co = coproto;
using co::task;

task<> t(std::string s)
{
    MC_BEGIN(task<>, s);

    std::cout << s << std::endl;

    MC_END();
}

task<> u()
{
    MC_BEGIN(
        task<>,
        a = macoro::eager_task<>{}
    );

    a = t("eager task in u") | macoro::make_eager();

    MC_AWAIT(a);

    MC_END();
}

int _main(int argc, char **argv)
{
    macoro::optional<macoro::eager_task<>> t1 = t("eager task") | macoro::make_eager();
    task<> t2 = t("normal task");
    task<> u1 = u();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::cout << "beep" << std::endl;

    if (t1)
    {
        co::sync_wait(*t1);
    }

    co::sync_wait(t2);
    co::sync_wait(u1);

    return 0;
}