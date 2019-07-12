#pragma once 
#include <nanodbc_ext/nanodbc_ext_config.h>
#include <nanodbc/nanodbc.h>
#ifndef LIBASYNC_STATIC
#define LIBASYNC_STATIC 1
#endif
#include <async++.h>
namespace nanodbc
{
async::shared_task<bool> async_connect(connection &con, const string &connection_string, long timeout = 0);
async::shared_task<void> async_prepare(statement &stm, const string &query, long timeout = 0);
async::shared_task<result> async_execute(statement &stm, long batch_operations = 1, long timeout = 0);
async::shared_task<result> async_execute_direct(statement &stm, class connection &conn, const string &query, long batch_operations = 1, long timeout = 0);
async::shared_task<bool> async_next(result &ret);
} // namespace nanodbc
#if NANODBC_HEADER_ONLY
#include <nanodbc_ext/detail/async_asyncplusplus-inl.h>
#endif