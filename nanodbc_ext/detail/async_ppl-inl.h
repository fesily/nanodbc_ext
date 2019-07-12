#pragma once 

#include <nanodbc_ext/detail/async_ppl.h>
#include <nanodbc_ext/detail/WinEvent.h>

#error msvc complier bug! I don't find any solution for this 
/*
    this is the bug code

concurrency::task<std::vector<std::tuple<std::string, std::string, std::string, std::string, std::string>>> next_cast_array(nanodbc::result& result)
{
    std::vector<std::tuple<std::string, std::string, std::string, std::string, std::string>> ret;
    if (result.columns() >= 5)
    {
        ret.reserve(result.rows());
        do
        {
            auto value = co_await next_cast<std::string, std::string, std::string, std::string, std::string>(result);
            if (!value.has_value())
                break;
            ret.push_back(std::move(value.value()));
        }
        while (true);
    }
    co_return ret;
}


concurrency::task<bool> AsyncMain()
{
    try
    {
        using namespace std::chrono_literals;
        nanodbc::connection conn;
        auto ret = co_await async_connect(conn, L"Driver={ODBC Driver 17 for SQL Server};Server=192.168.0.10;Database=;Uid=sa;Pwd=;", 30);
        auto factory = SqlStoredProcedure::QPAccountsDBFactory::CreateFactory();
        auto qpAccountDb =  factory->CreateQPAccountsDB(conn);
        auto [r, result] = co_await qpAccountDb->GSP_GP_LoadAccountsList();

        auto arr = co_await next_cast_array(result);
        co_return true;
    }
    catch (const nanodbc::database_error& e)
    {
        auto str = e.what();
    }
    catch (const std::exception& e)
    {
        auto str = e.what();
    }
    co_return false;
}

int main()
{
    int count = 9;
    while (count--)
    {
        AsyncMain().get();
    }
    return 0;
}    
 */

namespace nanodbc{

template <typename Result, typename Begin, typename Complete>
concurrency::task<Result> async_event_task(Begin&& begin, Complete&& complete)
{
    details::WinEvent event;
    if (begin(event))
    {
        assert(event != nullptr);

        concurrency::task_completion_event<Result>  promise;
        auto future = concurrency::create_task(promise);

        boost::winapi::HANDLE_ newWaitObject;
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
        return concurrency::task_from_result();
    }
    else
    {
        return concurrency::task_from_result(complete());
    }
}

NANODBC_INLINE concurrency::task<bool> async_connect(connection& con, const string& connection_string, long timeout)
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

NANODBC_INLINE concurrency::task<void> async_prepare(statement& stm, const string& query, long timeout)
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

NANODBC_INLINE concurrency::task<result> async_execute(statement& stm, long batch_operations, long timeout)
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

NANODBC_INLINE concurrency::task<result> async_execute_direct(statement& stm, class connection& conn, const string& query, long batch_operations, long timeout)
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

NANODBC_INLINE concurrency::task<bool> async_next(result& ret)
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
