#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "cacher.hpp"

#include <iostream>
#include <thread>

int MyFunction1(int n1, int n2)
{
    return n1 + n2;
}

int MyFunction2(std::string_view str)
{
    int count = 0;

    for (const auto &c : str)
    {
        count += static_cast<int>(c);
    }

    return count;
}

TEST_CASE("Cache return")
{
    CacherRegistry registry;
    auto c1 = registry.register_cacher(MyFunction1);
    auto c2 = registry.register_cacher(MyFunction2);

    const auto v1 = c1->get_value(2, 3);
    const auto v2 = c2->get_value("Hello");

    REQUIRE(v1 == 5);
    REQUIRE(v2 == 500);

    const auto v3 = c1->get_value(3, 2);

    REQUIRE(v3 == 5);
    REQUIRE(c1->get_hit_count() == 2);

    const auto v4 = c1->get_value(2, 3);
    REQUIRE(v4 == 5);
    REQUIRE(c1->get_hit_count() == 2);
}

TEST_CASE("Multi-threaded cache")
{
    CacherRegistry registry;
    auto c1 = registry.register_cacher(MyFunction1);
    auto c2 = registry.register_cacher(MyFunction2);
    auto c3 = registry.register_cacher(MyFunction2);

    auto multi_cache_1 = [&c1]() {
        for (int x = 0; x < 5; ++x)
        {
            for (int y = 0; y < 5; ++y)
            {
                const auto val1 = c1->get_value(x, y);
                const auto val2 = c1->get_value(y, x);
            }
        }
    };

    auto multi_cache_2 = [&c2]() {
        for (int x = 0; x < 5; ++x)
        {
            const auto val = c2->get_value("Hello");
        }
    };

    std::thread t1{multi_cache_1};
    std::thread t2{multi_cache_2};
    std::thread t3{multi_cache_2};
    std::thread t4{multi_cache_1};

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    REQUIRE(c1->get_hit_count() == 25);
    REQUIRE(c2->get_hit_count() == 1);
    REQUIRE(c2.use_count() == 2);
}

TEST_CASE("Multi-threaded registry")
{
    CacherRegistry registry;

    auto multi_reg_1 = [&registry]() {
        for (int x = 0; x < 5; ++x)
        {
            auto cacher = registry.register_cacher(MyFunction1);
        }
    };

    auto multi_reg_2 = [&registry]() {
        for (int x = 0; x < 5; ++x)
        {
            auto cacher = registry.register_cacher(MyFunction2);
        }
    };

    std::thread t1{multi_reg_1};
    std::thread t2{multi_reg_2};

    t1.join();
    t2.join();

    REQUIRE(registry.size() == 2);

    auto count_reg_1 = [&registry]() {
        constexpr auto num = 5;
        decltype(registry.register_cacher(MyFunction1)) cp[num];

        for (int x = 0; x < num; ++x)
        {
            cp[x] = registry.register_cacher(MyFunction1);
        }

        REQUIRE(registry.get_cacher_use_count(MyFunction1) == num);
    };

    std::thread t3{count_reg_1};

    t3.join();

    auto cx = registry.register_cacher(MyFunction1);
    REQUIRE(cx.use_count() == 1);
}
