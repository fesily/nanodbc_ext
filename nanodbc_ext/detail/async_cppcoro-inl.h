#pragma once 

#include <nanodbc_ext/detail/async_cppcoro.h>
#include <nanodbc_ext/detail/WinEvent.h>
#include <cppcoro/single_consumer_event.hpp>
#if defined(_WIN32) || defined(_WIN64)
#pragma comment(lib,"Synchronization.lib")
#endif

namespace nanodbc {

    template <typename Result, typename Begin, typename Complete, typename = std::enable_if_t<!std::is_void_v<Result>>>
    cppcoro::task<Result> async_event_task(Begin&& begin, Complete&& complete)
    {
        details::WinEvent event;
        Result result;
        if (begin(event))
        {
            assert(event != nullptr);
            boost::winapi::HANDLE_ newWaitObject;
            cppcoro::single_consumer_event consumer_event;
            std::exception_ptr exception_ptr;
            auto [fn, ptr] = GetRegisterWaitForSingleObjectFunc([&,complete=std::move(complete)]() mutable {
                try
                {
                    result = complete();
                }
                catch (...)
                {
                    exception_ptr = std::current_exception();
                }
                consumer_event.set();
            });
            boost::winapi::RegisterWaitForSingleObject(&newWaitObject, event, fn, ptr, -1, boost::winapi::WT_EXECUTEDEFAULT_ | boost::winapi::WT_EXECUTEONLYONCE_);

            co_await consumer_event;
            if(exception_ptr)
            {
                std::rethrow_exception(exception_ptr);
            }
            
            co_return result;
        }
        result = complete();
        co_return result;
    }

    template <typename Result,typename Begin, typename Complete,typename =std::enable_if_t<std::is_void_v<Result>>>
    cppcoro::task<void> async_event_task(Begin&& begin, Complete&& complete)
    {
        details::WinEvent event;
        if (begin(event))
        {
            assert(event != nullptr);
            boost::winapi::HANDLE_ newWaitObject;
            cppcoro::single_consumer_event consumer_event;
            std::exception_ptr exception_ptr;
            auto [fn, ptr] = GetRegisterWaitForSingleObjectFunc([&, complete = std::move(complete)]() mutable {
                try
                {
                    complete();
                }
                catch (...)
                {
                    exception_ptr = std::current_exception();
                }
                consumer_event.set();
            });
            boost::winapi::RegisterWaitForSingleObject(&newWaitObject, event, fn, ptr, -1, boost::winapi::WT_EXECUTEDEFAULT_ | boost::winapi::WT_EXECUTEONLYONCE_);
            co_await consumer_event;
            if(exception_ptr)
            {
                std::rethrow_exception(exception_ptr);
            }

            co_return;
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