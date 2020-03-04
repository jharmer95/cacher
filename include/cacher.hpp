#pragma once

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <typeindex>

template <typename T>
class Cacher;

template <typename R, typename... Args>
class Cacher<R(Args...)>
{
public:
    Cacher(const std::function<R(Args...)> &func) : calculator(func) {}
    Cacher(R (*func)(Args...)) : calculator(std::function<R(Args...)>(func)) {}

    R get_value(Args... args)
    {
        const auto tup = std::make_tuple(args...);

        {
            std::shared_lock rw_lock(mut);

            if (cached_vals.contains(tup))
            {
                return cached_vals[tup];
            }
        }

        return assign_value(args...);
    }

#ifndef NDEBUG
    size_t get_hit_count() const
    {
        return cache_hit_count;
    }
#endif

protected:
    R assign_value(Args... args)
    {
        const auto tup = std::make_tuple(args...);

        std::lock_guard rw_lock(mut);

        if (cached_vals.contains(tup))
        {
            return cached_vals[tup];
        }

#ifndef NDEBUG
        ++cache_hit_count;
#endif
        cached_vals[tup] = calculator(args...);
        return cached_vals[tup];
    }

    std::map<std::tuple<Args...>, R> cached_vals;
    std::function<R(Args...)> calculator;
    mutable std::shared_mutex mut;

#ifndef NDEBUG
    std::atomic<size_t> cache_hit_count = 0;
#endif
};

class CacherRegistry
{
public:
    template<typename R, typename... Args>
    std::shared_ptr<Cacher<R(Args...)>> register_cacher(R (*func)(Args...))
    {
        const auto fptr = static_cast<void*>(func);

        {
            std::shared_lock r_lock(mut);

            if (registry.contains(fptr) && !registry.at(fptr).expired())
            {
                return std::static_pointer_cast<Cacher<R(Args...)>>(std::shared_ptr<void>(registry[fptr]));
            }
        }

        auto ptr = std::make_shared<Cacher<R(Args...)>>(std::function<R(Args...)>(func));

        std::lock_guard rw_lock(mut);
        registry[fptr] = ptr;
        return ptr;
    }

    template<typename R, typename... Args>
    void deregister_cacher(R (*func)(Args...))
    {
        const auto fptr = static_cast<void*>(func);

        std::shared_lock r_lock(mut);
        registry.erase(fptr);
    }

    template<typename R, typename... Args>
    size_t get_cacher_use_count(R (*func)(Args...))
    {
        const auto fptr = static_cast<void*>(func);

        std::shared_lock r_lock(mut);
        
        if (registry.contains(fptr))
        {
            return registry[fptr].use_count();
        }

        return 0;
    }

    template<typename R, typename... Args>
    std::unique_ptr<Cacher<R(Args...)>> release_cacher(R (*func)(Args...))
    {
        deregister_cacher(func);
        return std::make_unique<Cacher<R(Args...)>>(func);
    }

    size_t size() const
    {
        std::shared_lock r_lock(mut);
        return registry.size();
    }

protected:
    std::map<void*, std::weak_ptr<void>> registry;
    mutable std::shared_mutex mut;
};
