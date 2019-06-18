#include <nanodbc_ext/nanodbc_ext.h>
#define FOLLY_HAS_COROUTINES 1
#define FOLLY_CORO_AWAIT_SUSPEND_NONTRIVIAL_ATTRIBUTES
#include <folly/futures/Future.h>
#include <folly/experimental/coro/Task.h>
folly::coro::Task<void> AsyncMain()
{
    try
    {
        //Driver={SQL Server};Server=myServerAddress;Database=myDataBase;
        //Trusted_Connection = Yes;
        using namespace std::chrono_literals;
        co_await folly::futures::sleep(1ms).via(&folly::InlineExecutor::instance());
        nanodbc::connection conn;
        bool ret = co_await async_connect(conn, L"Driver={ODBC Driver 17 for SQL Server};Server=127.0.0.1;Database=QPPlatformDB;Uid=sa;Pwd=1234;", 30);
        nanodbc::statement2 statement(conn);
        co_await async_prepare(statement, L"{? = call dbo.TestParam(?)}");
        statement.reset_parameters();

        int32_t ret1;
        statement.bind(0, &ret1, nanodbc::statement::param_direction::PARAM_RETURN);
        std::string p2 = "123445";
        statement.bind_string(1, p2, nanodbc::statement2::PARAM_INOUT);
        auto result = co_await async_execute(statement);
        while (co_await nanodbc::async_next(result))
        {
            auto id = result.get<int32_t>(0);
            auto addr = result.get<std::string>(L"DBAddr");
            auto dbport = result.get<std::string>(2);
            auto user = result.get<int32_t>(3);
            auto password = result.get<int32_t>(4);
            auto machineid = result.get<int32_t>(5);
            auto inform = result.get<std::string>(6);
        }
    }
    catch (const nanodbc::database_error& e)
    {
        auto str = e.what();
    }
    catch (const std::exception& e)
    {
        auto str = e.what();
    }
}

int main()
{
    AsyncMain().semi().get();
    return 0;
}