cmake_minimum_required(VERSION 3.8)

project(nanodbc_ext CXX)

option(NANODBC_DISABLE_INSTALL "Do not generate install target" OFF)
option(NANODBC_ASYNC_FOLLY "async future depend on folly" OFF)
option(NANODBC_ASYNC_PPL "async future depend on ppl" OFF)
option(NANODBC_ASYNC_ASYNCPLUSPLUS "async future depend on async++" OFF)
option(NANODBC_ASYNC_CPPCORO "async future depend on async++" OFF)
option(NANODBC_HEADER_ONLY "part of async is header only" OFF)

if(NOT NANODBC_ASYNC_FOLLY AND NOT NANODBC_ASYNC_PPL AND NOT NANODBC_ASYNC_ASYNCPLUSPLUS AND NOT NANODBC_ASYNC_CPPCORO)
  set(NANODBC_HEADER_ONLY ON)
endif()


########################################
## require and enable C++0x/11/14/17
########################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "nanodbc ext compile: c++ ${CMAKE_CXX_STANDARD}")


if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
  include(CheckCXXCompilerFlag)

  if (NANODBC_ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
    link_libraries(gcov)
    message(STATUS "nanodbc build: Enable test coverage - Yes")
  endif()

  if(NOT NANODBC_DISABLE_LIBCXX)
    check_cxx_compiler_flag("-stdlib=libc++" CXX_SUPPORTS_STDLIB)
    if(CXX_SUPPORTS_STDLIB)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
    endif()
    set(NANODBC_DISABLE_LIBCXX ${NANODBC_DISABLE_LIBCXX} CACHE BOOL "Do not use libc++, if available." FORCE)
  endif()
  message(STATUS "nanodbc build: Disable linking libc++ - ${NANODBC_DISABLE_LIBCXX}")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    string(REGEX REPLACE "[/-]W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (NOT (CMAKE_VERSION VERSION_LESS 3.6.0)) # Compiler features for Intel in CMake 3.6+
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++17")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /QaxCORE-AVX2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qipo")
elseif(MSVC)
  string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
  if(MSVC_VERSION LESS 1700)
    message(FATAL_ERROR, "nanodbc requires C++11-compliant compiler")
  endif()
endif()
set(BUILD_SRC_FILES nanodbc_ext/statement2.cpp)

if(NOT NANODBC_HEADER_ONLY)
  if(NANODBC_ASYNC_FOLLY)
    find_library(folly CONFIG REQUIRED)
    list(APPEND BUILD_SRC_FILES nanodbc_ext/detail/async_folly.cpp)
	include_directories(folly)
	link_libraries(folly)
  elseif(NANODBC_ASYNC_PPL)
    list(APPEND BUILD_SRC_FILES nanodbc_ext/detail/async_ppl.cpp)
  elseif(NANODBC_ASYNC_ASYNCPLUSPLUS)
    find_library(async++  CONFIG REQUIRED)
    list(APPEND BUILD_SRC_FILES nanodbc_ext/detail/async_asyncplusplus.cpp)
    include_directories(Async++)
    link_libraries(Async++)
  elseif(NANODBC_ASYNC_CPPCORO)
    find_library(cppcoro CONFIG REQUIRED)
    list(APPEND BUILD_SRC_FILES nanodbc_ext/detail/async_cppcoro.cpp)
    include_directories(cppcoro)
    link_libraries(cppcoro)
  else()
    message(FATAL_ERROR, "unkown async futuer lib")
  endif()
endif()

find_package(nanodbc CONFIG REQUIRED)

add_library(nanodbc_ext STATIC ${BUILD_SRC_FILES})

target_link_libraries(nanodbc_ext PRIVATE nanodbc)

target_include_directories(nanodbc_ext PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/nanodbc>)

if(UNIX)
  set_target_properties(nanodbc_ext PROPERTIES
    COMPILE_FLAGS "${ODBC_CFLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "lib")
endif()


if(NOT NANODBC_DISABLE_INSTALL)
    set(NANODBC_CONFIG nanodbc_ext-config)

    install(TARGETS nanodbc_ext 
		EXPORT ${NANODBC_CONFIG} # associate installed target files with export
        INCLUDES DESTINATION include
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

    install(FILES nanodbc_ext/nanodbc_ext_config.h DESTINATION include/nanodbc_ext)
    install(FILES nanodbc_ext/nanodbc_ext.h DESTINATION include/nanodbc_ext)
    install(FILES nanodbc_ext/nanodbc_async.h DESTINATION include/nanodbc_ext)
    install(FILES nanodbc_ext/statement2.h DESTINATION include/nanodbc_ext)
    install(FILES nanodbc_ext/nanodbc_boost_pfr_cast.hpp DESTINATION include/nanodbc_ext)
    install(FILES nanodbc_ext/nanodbc_cast.hpp DESTINATION include/nanodbc_ext)
    if(NANODBC_HEADER_ONLY)
      install(FILES nanodbc_ext/detail/WinEvent.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_ppl.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_ppl-inl.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_folly.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_folly-inl.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_asyncplusplus.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_asyncplusplus-inl.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_cppcoro.h DESTINATION include/nanodbc_ext/detail)
      install(FILES nanodbc_ext/detail/async_cppcoro-inl.h DESTINATION include/nanodbc_ext/detail)
	else()
	  if(NANODBC_ASYNC_FOLLY)
		install(FILES nanodbc_ext/detail/async_folly.h DESTINATION include/nanodbc_ext/detail)
	  elseif(NANODBC_ASYNC_PPL)
		install(FILES nanodbc_ext/detail/async_ppl.h DESTINATION include/nanodbc_ext/detail)
	  elseif(NANODBC_ASYNC_ASYNCPLUSPLUS)
      install(FILES nanodbc_ext/detail/async_asyncplusplus.h DESTINATION include/nanodbc_ext/detail)
    elseif(NANODBC_ASYNC_CPPCORO)
      install(FILES nanodbc_ext/detail/async_cppcoro.h DESTINATION include/nanodbc_ext/detail)
	  endif()
    endif()
	  install(EXPORT ${NANODBC_CONFIG} DESTINATION cmake)
    export(TARGETS nanodbc_ext FILE ${NANODBC_CONFIG}.cmake)
endif()
