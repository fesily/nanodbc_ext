
#include <nanodbc_ext/nanodbc_async.h>
#include <boost/winapi/handles.hpp>
#include <boost/winapi/event.hpp>
#include <boost/winapi/thread_pool.hpp>
using namespace nanodbc;

template <typename Func>
std::pair<boost::winapi::WAITORTIMERCALLBACK_, void *> GetRegisterWaitForSingleObjectFunc(Func &&func)
{
    auto cb = [](PVOID ptr, BOOLEAN) {
        std::unique_ptr<std::decay_t<Func>> context(static_cast<Func *>(ptr));
        (*context)();
    };
    return {cb, std::make_unique<std::decay_t<Func>>(std::forward<Func>(func)).release()};
}
namespace details
{
class WinEvent
{
public:
    explicit WinEvent()
        : event_(boost::winapi::create_anonymous_event(0, 0, 0))
    {
        ++refCount_;
    }

    WinEvent(const WinEvent &other)
        : event_(other.event_)
    {
        refCount_.store(++other.refCount_);
    }

    WinEvent &operator=(const WinEvent &other)
    {
        if (this == &other)
            return *this;
        event_ = other.event_;
        refCount_.store(++other.refCount_);
        return *this;
    }

    ~WinEvent()
    {
        if (1 == refCount_--)
        {
            if (refCount_.fetch_xor(std::numeric_limits<ptrdiff_t>::max()) == 0)
            {
                boost::winapi::CloseHandle(event_);
            }
        }
    }
    operator boost::winapi::HANDLE_() const { return event_; }

private:
    boost::winapi::HANDLE_ event_;
    mutable std::atomic_ptrdiff_t refCount_{};
};
} // namespace details
template <typename Result, typename Begin, typename Complete>
folly::Future<Result> async_event_task(Begin &&begin, Complete &&complete)
{
    details::WinEvent event;
    if (begin(event))
    {
        assert(event != nullptr);
        folly::Promise<Result> promise;
        boost::winapi::HANDLE_ newWaitObject;
        auto future = promise.getFuture().thenError(folly::tag_t<folly::FutureCancellation>{}, [=, ev = event](folly::FutureCancellation &&) mutable {
            boost::winapi::UnregisterWaitEx(newWaitObject, INVALID_HANDLE_VALUE);
            return complete();
            ;
        });
        auto [fn, ptr] = GetRegisterWaitForSingleObjectFunc([=, promise = std::move(promise), ev = event]() mutable {
            promise.setValue(complete());
        });
        boost::winapi::RegisterWaitForSingleObject(&newWaitObject, event, fn, ptr, INFINITE, WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);

        return future;
    }
    return folly::makeFuture<Result>(complete());
}

folly::Future<bool> async_connect(connection &con, const string &connection_string, long timeout = 0)
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

folly::Future<folly::Unit> async_prepare(statement &stm, const string &query, long timeout = 0)
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

folly::Future<result> async_execute(statement &stm, long batch_operations = 1, long timeout = 0)
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

folly::Future<result> async_execute_direct(statement &stm, class connection &conn, const string &query, long batch_operations = 1, long timeout = 0)
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

folly::Future<bool> async_next(result &ret)
{
    return async_event_task<bool>(
        [=](HANDLE event) mutable {
            return ret.async_next(event);
        },
        [=]() mutable {
            return ret.complete_next();
        });
}