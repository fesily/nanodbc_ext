#pragma once 

#include <nanodbc_ext/detail/async_folly.h>
#include <nanodbc_ext/detail/WinEvent.h>

namespace nanodbc{

template <typename Result, typename Begin, typename Complete>
folly::Future<Result> async_event_task(Begin &&begin, Complete &&complete)
{
    details::WinEvent event;
    if (begin(event))
    {
        assert(event != nullptr);
        folly::Promise<Result> promise;
        boost::winapi::HANDLE_ newWaitObject;
        auto future = promise.getFuture();
        auto [fn, ptr] = GetRegisterWaitForSingleObjectFunc([=, promise = std::move(promise), ev = event]() mutable {
            try
            {
                promise.setValue(complete());
            }
            catch(...)
            {
                promise.setException(folly::exception_wrapper::from_exception_ptr(std::current_exception()));
            }
        });
        boost::winapi::RegisterWaitForSingleObject(&newWaitObject, event, fn, ptr, INFINITE, WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);

        return future;
    }
    return folly::makeFuture<Result>(complete());
}

NANODBC_INLINE folly::Future<bool> async_connect(connection &con, const string &connection_string, long timeout)
{

    return async_event_task<bool>(
        [=](HANDLE event) mutable {
            return con.async_connect(connection_string, event, timeout);
        },
        [=]() mutable {
            con.async_complete();
            return con.connected();
        });
}

NANODBC_INLINE folly::Future<folly::Unit> async_prepare(statement &stm, const string &query, long timeout)
{
    return async_event_task<folly::Unit>(
        [=](HANDLE event) mutable {
            return stm.async_prepare(query, event, timeout);
        },
        [=]() mutable {
            stm.complete_prepare();
            return folly::Unit{};
        }

    );
}

NANODBC_INLINE folly::Future<result> async_execute(statement &stm, long batch_operations, long timeout)
{
    return async_event_task<result>(
        [=](HANDLE event) mutable {
            return stm.async_execute(event, batch_operations, timeout);
        },
        [=]() mutable {
            return stm.complete_execute(batch_operations);
        }

    );
}

NANODBC_INLINE folly::Future<result> async_execute_direct(statement &stm, class connection &conn, const string &query, long batch_operations, long timeout)
{
    return async_event_task<result>(
        [=](HANDLE event) mutable {
            return stm.async_execute_direct(const_cast<connection &>(conn), event, query, batch_operations, timeout);
        },

        [=]() mutable {
            return stm.complete_execute(batch_operations);
        }

    );
}

NANODBC_INLINE folly::Future<bool> async_next(result &ret)
{
    return async_event_task<bool>(
        [=](HANDLE event) mutable {
            return ret.async_next(event);
        },
        [=]() mutable {
            return ret.complete_next();
        });
}
}