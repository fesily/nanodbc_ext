#pragma once 
#include <nanodbc_ext/nanodbc_ext_config.h>
#include <nanodbc/nanodbc.h>
#include <folly/futures/Future.h>
namespace nanodbc
{
folly::Future<bool> async_connect(connection &con, const string &connection_string, long timeout = 0);
folly::Future<folly::Unit> async_prepare(statement &stm, const string &query, long timeout = 0);
folly::Future<result> async_execute(statement &stm, long batch_operations = 1, long timeout = 0);
folly::Future<result> async_execute_direct(statement &stm, class connection &conn, const string &query, long batch_operations = 1, long timeout = 0);
folly::Future<bool> async_next(result &ret);
} // namespace nanodbc
#if NANODBC_HEADER_ONLY
#include <nanodbc_ext/detail/async_folly-inl.h>
#endif