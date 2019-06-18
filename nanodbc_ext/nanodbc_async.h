#pragma once 
#ifndef NANODBC_ENABLE_UNICODE
#define NANODBC_ENABLE_UNICODE 1
#endif
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
