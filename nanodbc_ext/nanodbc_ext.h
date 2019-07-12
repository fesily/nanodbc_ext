#pragma once 
#include <nanodbc_ext/nanodbc_ext_config.h>
#include <nanodbc_ext/statement2.h>
#include <nanodbc_ext/nanodbc_async.h>
#ifdef _MSC_VER
#pragma comment(lib,"odbc32.lib")
#endif  

#if !defined(NANODBC_USE_BOOST_PFR) && defined( BOOST_PFR_DETAIL_CONFIG_HPP)
#define NANODBC_USE_BOOST_PFR 1
#endif

#if defined(NANODBC_USE_BOOST_PFR) && !defined(BOOST_PFR_DETAIL_CONFIG_HPP)
#include <boost/pfr.hpp>
#endif

#include <nanodbc/nanodbc_cast.hpp>