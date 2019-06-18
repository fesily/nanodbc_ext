#include <nanodbc_ext/statement2.h>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#include <sql.h>
#include <sqlext.h>

using namespace nanodbc;

size_t statement2::getParamBufferSize(short param_index, size_t char_size)
{
    SQLSMALLINT type;
    size_t size;
    this->disable_async();
    auto rc = ::SQLDescribeParam(this->native_statement_handle(), param_index + 1, &type, &size, 0, 0);
    if (rc != 0)
    {
        throw new nanodbc::database_error(this->native_statement_handle(), SQL_HANDLE_STMT, u8"无法访问的索引");
    }
    if (type == SQL_CHAR || type == SQL_VARCHAR || type == SQL_LONGVARCHAR || type == SQL_BINARY || type == SQL_VARBINARY || type == SQL_LONGVARBINARY)
    {
        return size;
    }
    if (type == SQL_WCHAR || type == SQL_WVARCHAR || type == SQL_WLONGVARCHAR)
    {
        return char_size == sizeof(char) ? size * 2 : size;
    }
    throw new nanodbc::programming_error(u8"错误的类型");
}