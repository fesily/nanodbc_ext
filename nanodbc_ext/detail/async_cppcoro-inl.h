#pragma once 

#include <nanodbc_ext/detail/async_cppcoro.h>
#include <nanodbc_ext/detail/WinEvent.h>
#include <cppcoro/single_consumer_event.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include <cppcoro/io_service.hpp>

#if defined(_WIN32) || defined(_WIN64)
#pragma comment(lib,"Synchronization.lib")
#endif

namespace nanodbc {

    struct schedule{
        static cppcoro::static_thread_pool& odbc_static_thread_pool()
        {
            static cppcoro::static_thread_pool pool(std::thread::hardware_concurrency() * 2 + 1);
            return pool;
        }
        static cppcoro::io_service& this_thread_io_service()
        {
            static thread_local cppcoro::io_service service;
            return service;
        }
        static void run_one()
        {
            this_thread_io_service().process_one_pending_event();
        }
        static void run()
        {
            this_thread_io_service().process_pending_events();
        }
    };

    template <typename Result, typename Begin, typename Complete, typename = std::enable_if_t<!std::is_void_v<Result>>>
    cppcoro::task<Result> async_event_task(Begin&& begin, Complete&& complete)
    {
        details::WinEvent event;
        if (begin(event))
        {
            assert(event != nullptr);
            auto& this_service = schedule::this_thread_io_service();
            co_await schedule::odbc_static_thread_pool().schedule();
            if (::WaitForSingleObject(event, timeout) != WAIT_TIMEOUT)
            {

            }
            co_await this_service.schedule();
        }
        co_return complete();
    }

    template <typename Result,typename Begin, typename Complete,typename =std::enable_if_t<std::is_void_v<Result>>>
    cppcoro::task<void> async_event_task(Begin&& begin, Complete&& complete)
    {
        details::WinEvent event;
        if (begin(event))
        {
            assert(event != nullptr);
            auto& this_service = schedule::this_thread_io_service();
            co_await schedule::odbc_static_thread_pool().schedule();
            if (::WaitForSingleObject(event, timeout) != WAIT_TIMEOUT)
            {

            }
            co_await this_service.schedule();
        }
        complete();
        co_return;
    }

    NANODBC_INLINE cppcoro::task<bool> async_connect(connection& con, const string& connection_string, long timeout)
    {
        co_return co_await async_event_task<bool>(
            [=](boost::winapi::HANDLE_ event) mutable {
                return con.async_connect(connection_string, event, timeout);
            },
            [=]() mutable {
                con.async_complete();
                return con.connected();
            });
    }

    NANODBC_INLINE cppcoro::task<void> async_prepare(statement& stm, const string& query, long timeout)
    {
        co_return co_await async_event_task<void>(
            [=](boost::winapi::HANDLE_ event) mutable {
                return stm.async_prepare(query, event, timeout);
            },
            [=]() mutable {
                stm.complete_prepare();
            }
            );
    }

    NANODBC_INLINE cppcoro::task<result> async_execute(statement& stm, long batch_operations, long timeout)
    {
        co_return co_await async_event_task<result>(
            [=](boost::winapi::HANDLE_ event) mutable {
                return stm.async_execute(event, batch_operations, timeout);
            },
            [=]() mutable {
                return stm.complete_execute(batch_operations);
            }
            );
    }

    NANODBC_INLINE cppcoro::task<result> async_execute_direct(statement& stm, class connection& conn, const string& query, long batch_operations, long timeout)
    {
        co_return co_await async_event_task<result>(
            [=](boost::winapi::HANDLE_ event) mutable {
                return stm.async_execute_direct(const_cast<connection&>(conn), event, query, batch_operations, timeout);
            },

            [=]() mutable {
                return stm.complete_execute(batch_operations);
            }

            );
    }

    NANODBC_INLINE cppcoro::task<bool> async_next(result& ret)
    {
        co_return co_await async_event_task<bool>(
            [=](boost::winapi::HANDLE_ event) mutable {
                return ret.async_next(event);
            },
            [=]() mutable {
                return ret.complete_next();
            });
    }
}