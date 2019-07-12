#pragma once 

#include <memory>
#include <optional>
#include <vector>
#include <cstdint>

#include <nanodbc/nanodbc.h>


namespace nanodbc
{
    using binary_t = std::vector<std::byte>;
    class statement2 : public statement
    {
    public:
        using statement::statement;
        using statement::bind;

        template<typename T,typename = std::enable_if_t<!std::is_pointer_v<T>>>
        void bind(short param_index, T const& value, [[maybe_unused]] param_direction direction = param_direction::PARAM_IN)
        {
            if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring>)
                this->bind_string(param_index, value, direction);
            else
                this->bind(param_index, std::addressof(value), direction);
        }

        template<typename T>
        void bind(short param_index, std::shared_ptr<T> const& value, param_direction direction = param_direction::PARAM_INOUT)
        {
            if (value != nullptr)
            {
                if constexpr(std::is_same_v<T,std::string> || std::is_same_v<T,std::wstring>)
                    this->bind_string(param_index, *value.get(), direction);
                else
                    this->bind(param_index, value.get(), direction);
            }
            else
                bind_null(param_index);
        }
        template<typename T>
        void bind(short param_index, std::optional<T> const& value, param_direction direction = param_direction::PARAM_IN)
        {
            if(value)
            {
                this->bind(param_index, value.value(), direction);
            }
            else
            {
                bind_null(param_index);
            }
        }
        template<typename T>
        void bind(short param_index, std::optional<std::shared_ptr<T>> const& value)
        {
            if (value)
            {
                this->bind(param_index, value.value(), param_direction::PARAM_INOUT);
            }
            else
            {
                bind_null(param_index);
            }
        }

        void bind(short param_index, std::vector<std::byte> const* value, param_direction direction = param_direction::PARAM_IN)
        {
            static_assert(sizeof(std::byte) == sizeof(char));
            statement::bind(param_index, reinterpret_cast<char const*>(value->data()), direction);
        }

        void bind(short param_index, int8_t const* value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, (const char*)value, direction);
        }

        void bind(short param_index, std::byte const* value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, reinterpret_cast<char const*>(value), direction);
        }

        void bind_string(short param_index,const std::string& value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, value.c_str(), value.size(), direction);
        }

        void bind_string(short param_index, const char* value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, value, std::char_traits<char>::length(value), direction);
        }

        void bind_string(short param_index, std::string& value,param_direction direction = param_direction::PARAM_IN)
        {
            if(direction != param_direction::PARAM_IN)
                value.reserve(getParamBufferSize(param_index,sizeof(std::string::value_type)));
            this->bind(param_index, value.c_str(), value.size(), direction);
        }

        void bind_string(short param_index, const char* value,size_t max_length,  param_direction direction = param_direction::PARAM_IN)
        {
            if (direction != param_direction::PARAM_IN)
                if (getParamBufferSize(param_index,sizeof(char)) > max_length)
                    BufferSmallException();
            this->bind(param_index, value, std::char_traits<char>::length(value), direction);
        }

        void bind_string(short param_index, const std::wstring& value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, value.c_str(), value.size(), direction);
        }

        void bind_string(short param_index, const wchar_t* value, param_direction direction = param_direction::PARAM_IN)
        {
            this->bind(param_index, value, std::char_traits<wchar_t>::length(value), direction);
        }

        void bind_string(short param_index, std::wstring& value, param_direction direction = param_direction::PARAM_IN)
        {
            if (direction != param_direction::PARAM_IN)
                value.reserve(getParamBufferSize(param_index,sizeof(std::wstring::value_type)));
            this->bind(param_index, value.c_str(), value.size(), direction);
        }

        void bind_string(short param_index, const wchar_t* value, size_t max_length, param_direction direction = param_direction::PARAM_IN)
        {
            if (direction != param_direction::PARAM_IN)
                if (getParamBufferSize(param_index,sizeof(wchar_t)) > max_length)
                    BufferSmallException();
            this->bind(param_index, value, std::char_traits<wchar_t>::length(value), direction);
        }

    private:
        size_t getParamBufferSize(short param_index, size_t char_size);
        [[noreturn]] void BufferSmallException()
        {
            throw new std::out_of_range(u8"buffer too small");
        }
    };
}