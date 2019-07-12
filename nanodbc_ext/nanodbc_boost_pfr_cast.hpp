#pragma once
#ifdef NANODBC_USE_BOOST_PFR

namespace nanodbc
{
    namespace detail
    {
        template<typename Tuple, uint8_t ...Index>
        void next_cast(Tuple& tuple,nanodbc::result& result, std::index_sequence<Index...>)
        {
            int arr[] = { (0,std::get<Index>(tuple) = result.get<std::tuple_element_t<Index,Tuple>>(Index))... };
            (void)arr;
        }

        template<typename ...Args>
        void next_cast(std::tuple<Args...>& tuple,nanodbc::result& result)
        {
            next_cast<std::tuple<Args...>>(tuple, result, std::index_sequence_for<Args...>{});
        }

    }

    template<typename T>
    future<std::optional<T>> next_cast_struct(nanodbc::result& result)
    {
        if (co_await async_next(result))
        {
            std::optional<T> value;
            value.emplace();
            co_return detail::next_cast(boost::pfr::structure_tie(value),result);
        }
        co_return std::nullopt;
    }

    template<typename T>
    future<std::vector<T>> next_cast_struct_array(nanodbc::result& result, size_t maxCount = std::numeric_limits<uint16_t>::max())
    {
        std::vector<T> ret;
        if (result.columns() >= boost::pfr::tuple_size_v<T>)
        {
            ret.reserve(result.rows());
            do
            {
                auto value = co_await next_cast_struct<T>(result);
                if (!value.has_value())
                    break;
                ret.push_back(std::move(value.value()));
            } while (maxCount--);
        }
        co_return ret;
    }
}
#endif