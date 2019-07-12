#pragma once 
#ifdef NANODBC_FUTURE_LIB_FOLLY
#include <nanodbc_ext/detail/async_folly.h>
#elif defined(NANODBC_FUTURE_LIB_ASYNCXX)
#include <nanodbc_ext/detail/async_asyncplusplus.h>
#elif defined(NANODBC_FUTURE_LIB_PPL)
#include <nanodbc_ext/detail/async_ppl.h>
#elif defined(NANODBC_FUTURE_LIB_CPPCORO)
#include <nanodbc_ext/detail/async_cppcoro.h>
#endif
