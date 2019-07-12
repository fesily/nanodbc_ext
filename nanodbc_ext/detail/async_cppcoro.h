#pragma once 
#include <nanodbc_ext/nanodbc_ext_config.h>
#include <nanodbc/nanodbc.h>
#include <cppcoro/task.hpp>
namespace nanodbc
{
    cppcoro::task<bool> async_connect(connection &con, const string &connection_string, long timeout = 0);
    cppcoro::task<void> async_prepare(statement &stm, const string &query, long timeout = 0);
    cppcoro::task<result> async_execute(statement &stm, long batch_operations = 1, long timeout = 0);
    cppcoro::task<result> async_execute_direct(statement &stm, class connection &conn, const string &query, long batch_operations = 1, long timeout = 0);
    cppcoro::task<bool> async_next(result &ret);
    template<typename T>
    using future = cppcoro::task<T>;
} // namespace nanodbc
#if NANODBC_HEADER_ONLY
#include <nanodbc_ext/detail/async_cppcoro-inl.h>
#endif