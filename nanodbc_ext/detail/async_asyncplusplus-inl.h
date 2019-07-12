#pragma once 

#include <nanodbc_ext/detail/async_asyncplusplus.h>
#include <nanodbc_ext/detail/WinEvent.h>

namespace nanodbc{

template <typename Result, typename Begin, typename Complete>
async::shared_task<Result> async_event_task(Begin&& begin, Complete&& complete)
{
    details::WinEvent event;
    if (begin(event))
    {
        assert(event != nullptr);
        async::event_task<Result> promise;
        boost::winapi::HANDLE_ newWaitObject;
        auto future = promise.get_task().share();
        auto [fn, ptr] = GetRegisterWaitForSingleObjectFunc([=, promise = std::move(promise), ev = event]() mutable {
            try
            {
                if constexpr(std::is_void_v<Result>)
                {
                    complete();
                    promise.set();
                }
                else
                {
                    promise.set(complete());
                }
            }
            catch(...)
            {
                promise.set_exception(std::current_exception());
            }
        });
        boost::winapi::RegisterWaitForSingleObject(&newWaitObject, event, fn, ptr, -1, boost::winapi::WT_EXECUTEDEFAULT_ | boost::winapi::WT_EXECUTEONLYONCE_);

        return future;
    }
    if constexpr(std::is_void_v<Result>)
    {
        complete();
        return async::make_task().share();
    }
    else
    {
        return async::make_task<Result>(complete()).share();
    }
}

NANODBC_INLINE async::shared_task<bool> async_connect(connection& con, const string& connection_string, long timeout)
{

    return async_event_task<bool>(
        [=](boost::winapi::HANDLE_ event) mutable {
            return con.async_connect(connection_string, event, timeout);
        },
        [=]() mutable {
            con.async_complete();
            return con.connected();
        });
}

NANODBC_INLINE async::shared_task<void> async_prepare(statement& stm, const string& query, long timeout)
{
    return async_event_task<void>(
        [=](boost::winapi::HANDLE_ event) mutable {
            return stm.async_prepare(query, event, timeout);
        },
        [=]() mutable {
            stm.complete_prepare();
        }
        );
}

NANODBC_INLINE async::shared_task<result> async_execute(statement& stm, long batch_operations, long timeout)
{
    return async_event_task<result>(
        [=](boost::winapi::HANDLE_ event) mutable {
            return stm.async_execute(event, batch_operations, timeout);
        },
        [=]() mutable {
            return stm.complete_execute(batch_operations);
        }

        );
}

NANODBC_INLINE async::shared_task<result> async_execute_direct(statement& stm, class connection& conn, const string& query, long batch_operations, long timeout)
{
    return async_event_task<result>(
        [=](boost::winapi::HANDLE_ event) mutable {
            return stm.async_execute_direct(const_cast<connection&>(conn), event, query, batch_operations, timeout);
        },

        [=]() mutable {
            return stm.complete_execute(batch_operations);
        }

        );
}

NANODBC_INLINE async::shared_task<bool> async_next(result& ret)
{
    return async_event_task<bool>(
        [=](boost::winapi::HANDLE_ event) mutable {
            return ret.async_next(event);
        },
        [=]() mutable {
            return ret.complete_next();
        });
}
}