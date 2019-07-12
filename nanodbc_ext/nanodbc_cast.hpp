#pragma once
#include <cstdint>
#include <tuple>
namespace nanodbc
{
    namespace detail
    {
        template<typename Tuple, uint8_t ...Index>
        Tuple next_cast(nanodbc::result& result, std::index_sequence<Index...>)
        {
            return { result.get<std::tuple_element_t<Index,Tuple>>(Index)... };
        }

        template<typename ...Args>
        std::tuple<Args...> next_cast(nanodbc::result& result)
        {
            return next_cast<std::tuple<Args...>>(result, std::index_sequence_for<Args...>{});
        }

    }

    template<typename ...Arg>
    future<std::optional<std::tuple<Arg...>>> next_cast_tuple(nanodbc::result& result)
    {
        if (co_await async_next(result))
        {
            co_return detail::next_cast<Arg...>(result);
        }
        co_return std::nullopt;
    }

    template<typename ...Arg>
    future<std::vector<std::tuple<Arg...>>> next_cast_tuple_array(nanodbc::result& result,size_t maxCount=std::numeric_limits<uint16_t>::max())
    {
        std::vector<std::tuple<Arg...>> ret;
        if (result.columns() >= sizeof...(Arg))
        {
            ret.reserve(result.rows());
            do
            {
                auto value = co_await next_cast_tuple<Arg...>(result);
                if (!value.has_value())
                    break;
                ret.push_back(std::move(value.value()));
            } while (maxCount--);
        }
        co_return ret;
    }
}
#include <nanodbc_ext/nanodbc_boost_pfr_cast.hpp>
