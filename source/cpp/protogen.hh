/*
 * Copyright 2023-2024 Bruno Costa <https://github.com/brunexgeek>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PROTOGEN_X_Y_Z
#define PROTOGEN_X_Y_Z

#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <iostream>
#include <forward_list>
#include <istream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <algorithm>
#include <cmath>

namespace protogen_X_Y_Z {

enum error_code
{
    PGERR_OK                = 0,
    PGERR_IGNORE_FAILED     = 1,
    PGERR_MISSING_FIELD     = 2,
    PGERR_INVALID_SEPARATOR = 3,
    PGERR_INVALID_VALUE     = 4,
    PGERR_INVALID_OBJECT    = 5,
    PGERR_INVALID_NAME      = 6,
    PGERR_INVALID_ARRAY     = 7,
};

enum parse_error
{
    PGR_OK,
    PGR_ERROR,
    PGR_NIL,
};

struct ErrorInfo
{
    error_code code;
    std::string message;
    int line, column;

    ErrorInfo() : code(error_code::PGERR_OK), line(0), column(0) {};
    ErrorInfo( error_code code, const std::string &message ) : code(code),
        message(message) { }
    ErrorInfo( error_code code, const std::string &message, int line, int column ) :
        code(code), message(message), line(line), column(column) { }
    ErrorInfo( const ErrorInfo &that ) = default;
    ErrorInfo( ErrorInfo &&that ) = default;
    operator bool() const
    {
        return code == error_code::PGERR_OK;
    }
    bool operator ==( error_code value ) const
    {
        return code == value;
    }
    bool operator ==( const ErrorInfo &that ) const
    {
        return code == that.code && line == that.line && column == that.column;
    }
    ErrorInfo &operator=( const ErrorInfo &that ) = default;
    void clear()
    {
        code = error_code::PGERR_OK;
        message.clear();
        line = column = 0;
    }
};

struct Parameters
{
    /// If true, ensures the output JSON will have all non-ASCII characters escaped.
    /// Default is false.
    bool ensure_ascii = false;

    bool serialize_null = false;

    /// Information about the error that occurred during the last operation.
    ErrorInfo error;
};

class ostream
{
    public:
        ostream() = default;
        virtual ~ostream() = default;
        virtual ostream &operator<<( const std::string &value ) = 0;
        virtual ostream &operator<<( const char *value ) = 0;
        virtual ostream &operator<<( char *value ) = 0;
        virtual ostream &operator<<( char value ) = 0;
        template<class T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
        ostream &operator<<( T value )
        {
            this->operator<<( std::to_string(value) );
            return *this;
        }
};

template<typename I>
class iterator_ostream : public ostream
{
    public:
        iterator_ostream( I& first ) : beg_(first)
        {
        }
        ostream & operator<<( char value ) override
        {
            *++beg_ = value;
            return *this;
        }
        ostream & operator<<( const std::string &value ) override
        {
            for (auto it = value.begin(); it != value.end(); ++it)
                *++beg_ = *it;
            return *this;
        }
        ostream & operator<<( const char *value ) override
        {
            while (*value != 0) *++beg_ = *value++;
            return *this;
        }
        ostream & operator<<( char *value ) override { return *this << (const char*) value; }

    protected:
        I beg_;
};

struct istream
{
    istream() = default;
    virtual ~istream() = default;
    virtual int peek() = 0;
    virtual void next() = 0;
    virtual bool eof() const = 0;
    virtual int line() const = 0;
    virtual int column() const = 0;
};

template<typename I>
class iterator_istream : public istream
{
    public:
        iterator_istream( const I& first, const I& last ) : beg_(first), end_(last), line_(1),
            column_(1)
        {
            skip();
        }
        int peek() override
        {
            if (beg_ == end_) return 0;
            return *beg_;
        }
        void next() override
        {
            if (beg_ == end_) return;
            ++beg_;
            ++column_;
            skip();
        }
        bool eof() const override { return beg_ == end_; }
        int line() const override { return line_; }
        int column() const override { return column_; }
    protected:
        I beg_, end_;
        int line_, column_;
        void skip()
        {
            while (!(beg_ == end_) && *beg_ == '\n')
            {
                ++line_;
                column_ = 1;
                ++beg_;
            }
        }
};

template<class T>
class mem_const_iterator
{
    static_assert(std::is_arithmetic<T>::value, "invalid template parameters");
    public:
        mem_const_iterator( const T *begin, size_t count ) : cursor(begin), end(begin + count), empty(0)
        {
        }
        mem_const_iterator &operator++()
        {
            if (cursor < end) cursor++;
            return *this;
        }
        const T &operator*() const
        {
            if (cursor >= end) return empty;
            return *cursor;
        }
        bool operator==( const mem_const_iterator<T> &that ) const
        {
            return cursor == that.cursor;
        }
    protected:
        const T *cursor;
        const T *end;
        T empty;
};

template<class T>
class mem_iterator
{
    static_assert(std::is_arithmetic<T>::value, "invalid template parameters");
    public:
        mem_iterator( T *begin, size_t count ) : cursor(begin), end(begin + count), empty(0)
        {
        }
        mem_iterator &operator++()
        {
            if (cursor < end) cursor++;
            return *this;
        }
        T &operator*()
        {
            if (cursor >= end) return empty;
            return *cursor;
        }
        bool operator==( const mem_iterator<T> &that ) const
        {
            return cursor == that.cursor;
        }
    protected:
        T *cursor;
        const T *end;
        T empty;
};

namespace internal {

using namespace ::protogen_X_Y_Z;

enum class token_id
{
    NONE, EOS, OBJS, OBJE, COLON, COMMA, STRING, ARRS,
    ARRE, NIL, BTRUE, BFALSE, NUMBER,
};

struct token
{
    token_id id;
    std::string value;
    int line, column;

    token() : id(token_id::NONE), line(0), column(0) {}
    token( const token &that ) { *this = that; }
    token( token &&that ) { swap(that); }
    token( token_id id, const std::string &value = "", int line = 0, int col = 0 ) : id(id), value(value),
        line(line), column(col) {}
    token &operator=( const token &that )
    {
        id = that.id;
        value = that.value;
        line = that.line;
        column = that.column;
        return *this;
    }
    void swap( token &that )
    {
        std::swap(id, that.id);
        value.swap(that.value);
        std::swap(line, that.line);
        std::swap(column, that.column);
    }
};

class tokenizer
{
    public:
        tokenizer( istream &input, Parameters &params ) : input_(input), error_(params.error)
        {
            next();
        }

        int line() const { return input_.line(); }
        int column() const { return input_.column(); }

        token &next()
        {
            #define RETURN_TOKEN(x) do { current_ = token(x, "", line, column); input_.next(); return current_; } while (false)
            current_.id = token_id::NONE;
            current_.value.clear();
            while (!input_.eof())
            {
                int c = input_.peek();
                int line = input_.line();
                int column = input_.column();
                switch (c)
                {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n':
                        input_.next();
                        break;
                    case '{':
                        RETURN_TOKEN(token_id::OBJS);
                    case '}':
                        RETURN_TOKEN(token_id::OBJE);
                    case '[':
                        RETURN_TOKEN(token_id::ARRS);
                    case ']':
                        RETURN_TOKEN(token_id::ARRE);
                    case ':':
                        RETURN_TOKEN(token_id::COLON);
                    case ',':
                        RETURN_TOKEN(token_id::COMMA);
                    case '"':
                        return current_ = parse_string();
                    case '-':
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        return current_ = parse_number();
                    default:
                        std::string value = parse_identifier();
                        if (value == "true") return current_ = token(token_id::BTRUE, "", line, column);
                        if (value == "false") return current_ = token(token_id::BFALSE, "", line, column);
                        if (value == "null") return current_ = token(token_id::NIL, "", line, column);
                        return current_ = token(token_id::NONE, "", line, column);
                }
            }
            return current_ = token(token_id::EOS, "", input_.line(), input_.column());
            #undef RETURN_TOKEN
        }
        token &peek() { return current_; }
        bool expect( token_id type )
        {
            if (current_.id == type)
            {
                next();
                return true;
            }
            return false;
        }
        int error( error_code code, const std::string &msg )
        {
            if (error_.code != error_code::PGERR_OK)
                return PGR_ERROR;
            error_.code = code;
            error_.message = msg;
            error_.line = current_.line;
            error_.column = current_.column;
            return PGR_ERROR;
        }
        void set_error(ErrorInfo &err)
        {
            error_ = err;
        }
        int ignore( ) { return ignore_value(); }

    protected:
        token current_;
        istream &input_;
        ErrorInfo &error_;

        std::string parse_identifier()
        {
            std::string value;
            while (!input_.eof())
            {
                int c = input_.peek();
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                {
                    value += (char) c;
                    input_.next();
                }
                else
                    break;
            }
            return value;
        }

        token parse_string()
        {
            int32_t lead = 0;
            std::string value;
            int line = input_.line();
            int column = input_.column();
            if (input_.peek() != '"') goto ESCAPE;
            while (!input_.eof())
            {
                input_.next();
                int c = input_.peek();
                if (c == '"')
                {
                    input_.next();
                    return token(token_id::STRING, value);
                }
                if (c == '\\')
                {
                    input_.next();
                    c = input_.peek();
                    switch (c)
                    {
                        case '"':  value += '"'; break;
                        case '\\': value += '\\'; break;
                        case '/':  value += '/'; break;
                        case 'b':  value += '\b'; break;
                        case 'f':  value += '\f'; break;
                        case 'r':  value += '\r'; break;
                        case 'n':  value += '\n'; break;
                        case 't':  value += '\t'; break;
                        case 'u':
                            if (!parse_escaped_utf8(value, lead))
                                goto ESCAPE;
                            break;
                        default: goto ESCAPE;
                    }
                }
                else
                {
                    if (c == 0)
                        goto ESCAPE;
                    value += (char) c;
                }
            }
            ESCAPE:
            return token(token_id::NONE, "", line, column);
        }

        bool parse_escaped_utf8(std::string &value, int32_t &lead)
        {
            char temp[5] = {0};
            for (int i = 0; i < 4; ++i)
            {
                input_.next();
                auto c = input_.peek();
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
                    temp[i] = (char) c;
                else
                    return false;
            }
            int32_t codepoint = (int32_t) strtol(temp, nullptr, 16);

            // first value in UTF-16 surrogate pair
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
            {
                lead = codepoint;
                return true;
            }
            else
            // second value in UTF-16 surrogate pair
            if (codepoint >= 0xDC00 && codepoint <= 0xDFFF)
            {
                // check whether we have a lead (first value in the surrogate pair)
                if (lead == 0)
                    return false;
                // compute the final codepoint
                static const int32_t SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;
                codepoint = (lead << 10) + codepoint + SURROGATE_OFFSET;

                // 4-byte UTF-8 = 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                value += (char) (0xF0 | ((codepoint >> 18) & 0x07));
                value += (char) (0x80 | ((codepoint >> 12) & 0x3F));
                value += (char) (0x80 | ((codepoint >> 6) & 0x3F));
                value += (char) (0x80 | (codepoint & 0x3F));
            }
            else
            // 2-byte UTF-8 = 110xxxxx 10xxxxxx
            if (codepoint >= 0x80 && codepoint <= 0x7FF)
            {
                value += (char) (0xC0 | ((codepoint >> 6) & 0x1F));
                value += (char) (0x80 | (codepoint & 0x3F));
            }
            else
            // 3-byte UTF-8 = 1110xxxx 10xxxxxx 10xxxxxx
            if (codepoint >= 0x800 && codepoint <= 0xFFFF)
            {
                value += (char) (0xE0 | ((codepoint >> 12) & 0x0F));
                value += (char) (0x80 | ((codepoint >> 6) & 0x3F));
                value += (char) (0x80 | (codepoint & 0x3F));
            }
            else
                return false;

            // reset the surrogate pair lead
            lead = 0;

            return true;
        }

        bool parse_keyword( const std::string &keyword )
        {
            for (auto c : keyword)
            {
                if (input_.peek() != c) return false;
                input_.next();
            }
            return true;
        }

        token parse_number() // TODO ensure number syntax according to RFC-4627 section 2.4
        {
            std::string value;
            int line = input_.line();
            int column = input_.column();
            while (!input_.eof())
            {
                int c = input_.peek();
                if (c == '.' || (c >= '0' && c <= '9') || c == 'e' || c == 'E' || c == '+' || c == '-')
                {
                    value += (char) c;
                    input_.next();
                }
                else
                    break;
            }
            return token(token_id::NUMBER, value, line, column);
        }

        int ignore_array()
        {
            if (!expect(token_id::ARRS))
                return error(error_code::PGERR_IGNORE_FAILED, "invalid array");

            while (peek().id != token_id::ARRE)
            {
                int result = ignore_value();
                if (result != PGERR_OK) return result;
                if (!expect(token_id::COMMA)) break;
            }
            if (!expect(token_id::ARRE))
                return error(error_code::PGERR_IGNORE_FAILED, "invalid array");
            return PGR_OK;
        }

        int ignore_object()
        {
            if (!expect(token_id::OBJS))
                return error(error_code::PGERR_IGNORE_FAILED, "invalid object");

            while (peek().id != token_id::OBJE)
            {
                if (!expect(token_id::STRING))
                    return error(error_code::PGERR_IGNORE_FAILED, "expected field name");
                if (!expect(token_id::COLON))
                    return error(error_code::PGERR_IGNORE_FAILED, "expected colon");
                int result = ignore_value();
                if (result != PGERR_OK) return result;
                if (!expect(token_id::COMMA)) break;
            }
            if (!expect(token_id::OBJE))
                return error(error_code::PGERR_IGNORE_FAILED, "invalid object");
            return PGR_OK;
        }

        int ignore_value()
        {
            switch (peek().id)
            {
                case token_id::NONE:
                case token_id::EOS:
                    return error(error_code::PGERR_IGNORE_FAILED, "end of stream");
                case token_id::OBJS:
                    return ignore_object();
                case token_id::ARRS:
                    return ignore_array();
                case token_id::STRING:
                case token_id::NUMBER:
                case token_id::NIL:
                case token_id::BTRUE:
                case token_id::BFALSE:
                {
                    auto tt = next();
                    if (tt.id == token_id::NONE || tt.id == token_id::EOS)
                        return error(PGERR_IGNORE_FAILED, "end of stream");
                    return PGERR_OK;
                }
                default:
                    return error(error_code::PGERR_IGNORE_FAILED, "invalid value");
            }
        }
};

} // namespace internal

template <typename T>
#if !defined(_WIN32)
constexpr
#endif
T rol( T value, int count )
{
	static_assert(std::is_unsigned<T>::value, "unsupported signed type");
	return (T) ((value << count) | (value >> (-count & (sizeof(T) * 8 - 1))));
}

static inline std::string reveal( const std::string &value )
{
    uint8_t mask = rol<uint8_t>(0x93U, value.length() % 8);
	std::string result;
    result.reserve(value.length());
	for (size_t i = 0; i < value.length(); ++i)
		result += (char) ((uint8_t) value[i] ^ mask);
	return result;
}

// Parent class for messages
template<typename T, typename J>
struct message
{
    typedef T underlying_type;
    typedef J serializer_type;
    virtual ~message() = default;
    virtual bool deserialize( istream &in, Parameters *params = nullptr ) = 0;
    virtual bool serialize( ostream &out, Parameters *params = nullptr ) const = 0;

    virtual bool deserialize( std::istream &in, Parameters *params = nullptr )
    {
        bool skip = in.flags() & std::ios_base::skipws;
        std::noskipws(in);
        std::istream_iterator<char> end;
        std::istream_iterator<char> begin(in);
        iterator_istream<std::istream_iterator<char>> is(begin, end);
        bool result = deserialize(is, params);
        if (skip) std::skipws(in);
        return result;
    }

    virtual bool deserialize( const std::string &in, Parameters *params = nullptr )
    {
        iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
        return deserialize(is, params);
    }

    virtual bool deserialize( const char *in, size_t len, Parameters *params = nullptr )
    {
        auto begin = mem_const_iterator<char>(in, len);
        auto end = mem_const_iterator<char>(in + len, 0);
        iterator_istream<mem_const_iterator<char>> is(begin, end);
        return deserialize(is, params);
    }

    virtual bool deserialize( const std::vector<char> &in, Parameters *params = nullptr )
    {
        iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
        return deserialize(is, params);
    }

    virtual bool serialize( std::string &out, Parameters *params = nullptr ) const
    {
        typedef std::back_insert_iterator<std::string> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        return serialize(os, params);
    }

    virtual bool serialize( std::ostream &out, Parameters *params = nullptr ) const
    {
        typedef std::ostream_iterator<char> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        return serialize(os, params);
    }

    virtual bool serialize( char *in, size_t len, Parameters *params = nullptr )
    {
        auto begin = mem_iterator<char>(in, len);
        iterator_ostream<mem_iterator<char>> os(begin);
        return serialize(os, params);
    }

    virtual bool serialize( std::vector<char> &out, Parameters *params = nullptr ) const
    {
        typedef std::back_insert_iterator<std::vector<char>> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        return serialize(os, params);
    }

    virtual void clear() = 0;
    virtual bool empty() const  = 0;
    virtual bool equal( const T &that ) const = 0;
    bool operator==( const T &that ) const { return equal(that); }
    bool operator!=( const T &that ) const { return !equal(that); }
};

} // namespace protogen_X_Y_Z

#endif // PROTOGEN_X_Y_Z
