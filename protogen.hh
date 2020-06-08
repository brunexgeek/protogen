#ifndef PROTOGEN_2_0_0
#define PROTOGEN_2_0_0

#include <string>
#include <vector>
#include <iostream>
#include <forward_list>
#include <istream>
#include <iterator>

#define MAKE_STRING(...) #__VA_ARGS__

namespace protogen_2_0_0 {

enum class error_code
{
    PGERR_IGNORE_FAILED     = -1,
    PGERR_MISSING_FIELD     = -2,
    PGERR_INVALID_SEPARATOR = -3,
    PGERR_INVALID_VALUE     = -4,
    PGERR_INVALID_OBJECT    = -5,
    PGERR_INVALID_NAME      = -6,
};

class exception : public std::exception
{
    protected:
        bool empty_;
        error_code code_;
        std::string msg_;
        int line_, column_;
    public:
        exception() : empty_(true) {};
        exception( error_code code, const std::string &msg ) : empty_(false), code_(code), msg_(msg) { }
        exception( error_code code, const std::string &msg, int line, int column ) : empty_(false),
          code_(code), msg_(msg), line_(line), column_(column)
        {
            msg_ += " at ";
            msg_ += std::to_string(line_);
            msg_ += ':';
            msg_ += std::to_string(column_);
        }
        exception( const exception &that ) : empty_(that.empty_), code_(that.code_), msg_(that.msg_),
            line_(that.line_), column_(that.column_) { }
        exception( exception &&that )
        {
            msg_.swap(that.msg_);
            std::swap(code_, that.code_);
            std::swap(line_, that.line_);
            std::swap(column_, that.column_);
            std::swap(empty_, that.empty_);
        }
        error_code code() const { return code_; }
        virtual const char *what() const noexcept override { return msg_.c_str(); }
        operator bool() const { return empty_; }
        bool operator ==( error_code value ) const { return code_ == value; }
};

template<typename T>
static std::string reveal( const T *value, size_t length )
{
    std::string result(length, ' ');
    for (size_t i = 0; i < length; ++i)
        result[i] = (T) ((int) value[i] ^ 0x33);
    return result;
}


namespace internal {

enum class token_id
{
    NONE, EOS, OBJS, OBJE, COLON, COMMA, STRING, ARRS,
    ARRE, NIL, TRUE, FALSE, NUMBER,
};

struct token
{
    token_id id = token_id::NONE;
    std::string value;

    token() {}
    token( const token &that ) : id(that.id), value(that.value) {}
    token( token &&that ) { swap(that); }
    token( token_id id, const std::string &value = "" ) : id(id), value(value) { }
    token &operator=( const token &that ) { id = that.id, value = that.value; return *this; }
    void swap( token &that ) { std::swap(id, that.id); value.swap(that.value); }
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
        template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
        ostream &operator<<( T value )
        {
            this->operator<<( std::to_string((double)value) );
            return *this;
        }
        template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        ostream &operator<<( T value )
        {
            this->operator<<( std::to_string( (T) ((double) value) ) );
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

class istream
{
    public:
        istream() = default;
        virtual ~istream() = default;
        virtual int peek() = 0;
        virtual int get() = 0;
        virtual bool eof() const = 0;
        virtual int line() const = 0;
        virtual int column() const = 0;
};

template<typename I>
class iterator_istream : public istream
{
    public:
        iterator_istream( const I& first, const I& last ) : beg_(first), end_(last), line_(1),
            column_(0)
        {
        }
        int peek() override
        {
            if (beg_ == end_) return 0;
            return *beg_;
        }
        int get() override
        {
            if (beg_ == end_) return 0;
            ++column_;
            int c = *beg_;
            ++beg_;
            if (c == '\n')
            {
                ++line_;
                column_ = 0;
            }
            return c;
        }
        bool eof() const override { return beg_ == end_; }
        int line() const override { return line_; }
        int column() const override { return column_; }
    protected:
        I beg_, end_;
        int line_, column_;
};

class tokenizer
{
    public:
        tokenizer( istream &input ) : input_(input)
        {
            next();
        }

        int line() const { return input_.line(); }
        int column() const { return input_.column(); }

        token &next()
        {
            if (!stack_.empty())
            {
                current_ = stack_.front();
                stack_.pop_front();
                return current_;
            }
            current_.id = token_id::NONE;
            current_.value.clear();
            while (!input_.eof())
            {
                int c = input_.get();
                switch (c)
                {
                    case ' ':
                    case '\t':
                    case '\r':
                    case '\n':
                        break;
                    case '{': return current_ = token(token_id::OBJS);
                    case '}': return current_ = token(token_id::OBJE);
                    case '[': return current_ = token(token_id::ARRS);
                    case ']': return current_ = token(token_id::ARRE);
                    case ':': return current_ = token(token_id::COLON);
                    case ',': return current_ = token(token_id::COMMA);
                    case '"':
                        parse_string(current_);
                        return current_;
                    case 'n':
                        if (!parse_keyword("ull")) return current_;
                        return current_ = token(token_id::NIL);
                    case 't':
                        if (!parse_keyword("rue")) return current_;
                        return current_ = token(token_id::TRUE);
                    case 'f':
                        if (!parse_keyword("alse")) return current_;
                        return current_ = token(token_id::FALSE);
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
                        current_.value += (char) c;
                        if (!parse_number(current_)) return current_;
                        return current_;
                    default:
                        return current_ = token(token_id::NONE);
                }
            }

            return current_ = token(token_id::EOS);
        }
        void unnext()
        {
            token temp;
            temp.swap(current_);
            stack_.push_front(temp);
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
        void error( error_code code, const std::string &msg ) const { throw exception(code, msg, input_.line(), input_.column()); }
        void ignore( ) { ignore_value(); }

    protected:
        token current_;
        istream &input_;
        std::forward_list<token> stack_;

        bool parse_string( token &output )
        {
            output.id = token_id::STRING;

            while (!input_.eof())
            {
                int c = input_.get();
                if (c == '"') return true;

                if (c == '\\')
                {
                    c = input_.get();
                    switch (c)
                    {
                        case '"':  c = '"'; break;
                        case '\\': c = '\\'; break;
                        case '/':  c = '/'; break;
                        case 'b':  c = '\b'; break;
                        case 'f':  c = '\f'; break;
                        case 'r':  c = '\r'; break;
                        case 'n':  c = '\n'; break;
                        case 't':  c = '\t'; break;
                        default: return false;
                    }
                }

                if (c <= 0) return false;
                output.value += (char) c;
            }

            return false;
        }

        bool parse_keyword( const std::string &keyword )
        {
            for (auto c : keyword)
                if (input_.get() != c) return false;
            return true;
        }

        bool parse_number( token &output )
        {
            output.id = token_id::NUMBER;

            while (!input_.eof())
            {
                int c = input_.peek();
                if (c == '.' || (c >= '0' && c <= '9') || c == 'e' || c == 'E' || c == '+' || c == '-')
                {
                    output.value += (char) c;
                    input_.get();
                }
                else
                    break;
            }

            return true;
        }

        void ignore_array()
        {
            if (!expect(token_id::ARRS))
                error(error_code::PGERR_IGNORE_FAILED, "Invalid array");

            while (true)
            {
                ignore_value();
                if (!expect(token_id::COMMA)) break;
            }
            if (!expect(token_id::ARRE))
                error(error_code::PGERR_IGNORE_FAILED, "Invalid array");
        }

        void ignore_object()
        {
            if (!expect(token_id::OBJS))
                error(error_code::PGERR_IGNORE_FAILED, "Invalid object");

            while (true)
            {
                if (!expect(token_id::STRING))
                    error(error_code::PGERR_IGNORE_FAILED, "Expected field name");
                if (!expect(token_id::COLON))
                    error(error_code::PGERR_IGNORE_FAILED, "Expected colon");
                ignore_value();
                if (!expect(token_id::COMMA)) break;
            }
            if (!expect(token_id::OBJE))
                error(error_code::PGERR_IGNORE_FAILED, "Invalid object");
        }

        void ignore_value()
        {
            switch (peek().id)
            {
                case token_id::NONE:
                    error(error_code::PGERR_IGNORE_FAILED, "Invalid json");
                    break;
                case token_id::OBJS:
                    ignore_object();
                    break;
                case token_id::ARRS:
                    ignore_array();
                    break;
                case token_id::STRING:
                case token_id::NUMBER:
                case token_id::NIL:
                case token_id::TRUE:
                case token_id::FALSE:
                    next();
                    break;
                default:
                    error(error_code::PGERR_IGNORE_FAILED, "Invalid json");
            }
        }
};

template<class T>
class mem_iterator
{
    static_assert(std::is_arithmetic<T>::value, "Invalid template parameters");
    public:
        mem_iterator( const T *begin, size_t count ) : cursor(begin), end(begin + count), empty(0)
        {
        }
        mem_iterator &operator++()
        {
            if (cursor < end) cursor++;
            return *this;
        }
        const T &operator*() const
        {
            if (cursor >= end) return empty;
            return *cursor;
        }
        bool operator==( const mem_iterator<T> &that ) const
        {
            return cursor == that.cursor;
        }
    protected:
        const T *cursor;
        const T *end;
        T empty;
};

} // namespace internal

using namespace protogen_2_0_0::internal;

template<typename T, typename _ = void>
struct is_container : std::false_type {};

template<typename... Ts>
struct is_container_helper {};

template<typename T>
struct is_container<
        T,
        typename std::conditional<
            false,
            is_container_helper<
                typename T::value_type,
                typename T::size_type,
                typename T::allocator_type,
                typename T::iterator,
                typename T::const_iterator,
                decltype(std::declval<T>().size()),
                decltype(std::declval<T>().begin()),
                decltype(std::declval<T>().end()),
                decltype(std::declval<T>().clear()),
                decltype(std::declval<T>().empty())
                >,
            void
            >::type
        > : public std::true_type {};

template<typename T, typename E = void> struct json;

template<typename T> class field
{
    static_assert(std::is_arithmetic<T>::value, "Invalid arithmetic type");
    protected:
        T value_;
        bool empty_;
    public:
        typedef T value_type;
        field() { clear(); }
        field( const field<T> &that ) { this->empty_ = that.empty_; if (!empty_) this->value_ = that.value_; }
        field( field<T> &&that ) { this->empty_ = that.empty_; if (!empty_) this->value_.swap(that.value_); }
        void swap( field<T> &that ) { std::swap(this->value_, that.value_); std::swap(this->empty_, that.empty_); }
        void swap( T &that ) { std::swap(this->value_, that); empty_ = false; }
        const T operator()() const { return value_; }
        void operator()(const T &value ) { this->value_ = value; this->empty_ = false; }
        bool empty() const { return empty_; }
        void clear() { value_ = (T) 0; empty_ = true; }
        field<T> &operator=( const field<T> &that ) { this->empty_ = that.empty_; if (!empty_) this->value_ = that.value_; return *this; }
        field<T> &operator=( const T &that ) { this->empty_ = false; this->value_ = that; return *this; }
        bool operator==( const T &that ) const { return !this->empty_ && this->value_ == that; }
        bool operator!=( const T &that ) const { return !this->empty_ && this->value_ != that; }
        bool operator==( const field<T> &that ) const { return this->empty_ == that.empty_ && this->value_ == that.value_;  }
        bool operator!=( const field<T> &that ) const { return this->empty_ != that.empty_ || this->value_ != that.value_;  }
        operator T() const { return this->value_; }
};

template<typename T>
struct json<field<T>, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    static void read( tokenizer &tok, field<T> &value )
    {
        T temp;
        json<T>::read(tok, temp);
        value = temp;
    }
    static void write( ostream &os, const field<T> &value )
    {
        T temp = value();
        json<T>::write(os, temp);
    }
    static bool empty( const field<T> &value ) { return value.empty(); }
    static void clear( field<T> &value ) { value.clear(); }
};

template<typename T>
struct json<T, typename std::enable_if<std::is_arithmetic<T>::value>::type >
{
    static void read( tokenizer &tok, T &value )
    {
        auto &tt = tok.peek();
        if (tt.id == token_id::NIL) return;
        if (tt.id != token_id::NUMBER)
            tok.error(error_code::PGERR_INVALID_VALUE, "Invalid numeric value");
        //std::cerr << "Parsing '" << tt.value << "'\n";
#if defined(_WIN32) || defined(_WIN64)
        static _locale_t loc = _create_locale(LC_NUMERIC, "C");
        if (loc == NULL) tok.error(error_code::PGERR_INVALID_VALUE, "Invalid locale");
        value = static_cast<T>(_strtod_l(tt.value.c_str(), NULL, loc));
#else
        static locale_t loc = newlocale(LC_NUMERIC_MASK, "C", 0);
        if (loc == 0) tok.error(error_code::PGERR_INVALID_VALUE, "Invalid locale");
#ifdef __USE_GNU
        value = static_cast<T>(strtod_l(tt.value.c_str(), NULL, loc));
#else
        locale_t old = uselocale(loc);
        if (old == 0) tok.error(error_code::PGERR_INVALID_VALUE, "Unable to set locale");
        value = static_cast<T>(strtod(tt.value.c_str(), NULL));
        uselocale(old);
#endif
#endif
        tok.next();
    }
    static void write( ostream &os, const T &value ) { os << value; }
    static bool empty( const T &value ) { (void) value; return false; }
    static void clear( T &value ) { value = (T) 0; }
};

template<typename T>
struct json<T, typename std::enable_if<is_container<T>::value>::type >
{
    static void read( tokenizer &tok, T &value )
    {
        if (tok.expect(token_id::NIL)) return;
        if (!tok.expect(token_id::ARRS))
            tok.error(error_code::PGERR_INVALID_OBJECT, "Invalid object");
        while (true)
        {
            typename T::value_type temp;
            json<typename T::value_type>::read(tok, temp);
            value.push_back(temp);
            if (!tok.expect(token_id::COMMA)) break;
        }
        if (!tok.expect(token_id::ARRE))
            tok.error(error_code::PGERR_INVALID_OBJECT, "Invalid object");
    }
    static void write( ostream &os, const T &value )
    {
        os << '[';
        size_t i = 0, t = value.size();
        for (auto it = value.begin(); it != value.end(); ++it, ++i)
        {
            json<typename T::value_type>::write(os, *it);
            if (i + 1 < t) os << ',';
        }
        os << ']';
    }
    static bool empty( const T &value ) { return value.empty(); }
    static void clear( T &value ) { value.clear(); }
};


// Base64 encoder/decoder based on Joe DF's implementation
// Original source at <https://github.com/joedf/base64.c> (MIT licensed)
template <>
struct json< std::vector<uint8_t> >
{
    static int b64_int( int ch )
    {
        if (ch == '+') return 62;
        if (ch == '/') return 63;
        if (ch == '=') return 64;
        if (ch >= '0' && ch <= '9') return ch + 4;
        if (ch >= 'A' && ch <= 'Z') return ch - 'A';
        if (ch >= 'a' && ch <= 'z') return (ch - 'a') + 26;
        return 0;
    }
    static void write( ostream &out, const std::vector<uint8_t> &value )
    {
        static const char *B64_SYMBOLS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        char o[5] = { 0 };
        size_t i = 0;
        size_t size = value.size();

        out << '"';

        for (i = 0; i + 2 < size; i += 3)
        {
            o[0] = B64_SYMBOLS[ (value[i] & 0xFF) >> 2 ];
            o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) | ((value[i + 1] & 0xF0) >> 4) ];
            o[2] = B64_SYMBOLS[ ((value[i+1] & 0x0F) << 2) | ((value[i+2] & 0xC0) >> 6) ];
            o[3] = B64_SYMBOLS[ value[i+2] & 0x3F ];
            out << o;
        }

        if (size - i)
        {
            o[0] = B64_SYMBOLS[ (value[i] & 0xFF) >> 2 ];
            o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) ];
            o[2] = '=';
            o[3] = '=';

            if (size - i == 2)
            {
                o[1] = B64_SYMBOLS[ ((value[i] & 0x03) << 4) | ((value[i + 1] & 0xF0) >> 4) ];
                o[2] = B64_SYMBOLS[ ((value[i+1] & 0x0F) << 2) ];
            }

            out << o;
        }
        out << '"';
    }
    static void read( tokenizer &tok, std::vector<uint8_t> &value )
    {
        if (tok.expect(token_id::NIL)) return;
        if (tok.peek().id != token_id::STRING)
            tok.error(error_code::PGERR_INVALID_OBJECT, "Invalid string");

        size_t k = 0;
        int s[4];
        std::string text = tok.peek().value;
        tok.next();
        const char *ptr = text.c_str();

        while (true)
        {
            // read 4 characters
            for (size_t j = 0; j < 4; ++j)
            {
                int ch = *ptr++;
                if (ch == 0)
                {
                    if (j != 0) tok.error(error_code::PGERR_INVALID_OBJECT, "Invalid base64 data");
                    return;
                }
                s[j] = b64_int(ch);
            }
            // decode base64 tuple
            value.push_back( (uint8_t) (((s[0] & 0xFF) << 2 ) | ((s[1] & 0x30) >> 4)) );
            if (s[2] != 64)
            {
                value.push_back( (uint8_t) (((s[1] & 0x0F) << 4) | ((s[2] & 0x3C) >> 2)) );
                if ((s[3]!=64))
                {
                    value.push_back( (uint8_t) (((s[2] & 0x03) << 6) | s[3]) );
                    k+=3;
                }
                else
                    k+=2;
            }
            else
                k+=1;
        }
    }
    static bool empty( const std::vector<uint8_t> &value ) { return value.empty(); }
    static void clear( std::vector<uint8_t> &value ) { value.clear(); }
};

template<>
struct json<bool, void>
{
    static void read( tokenizer &tok, bool &value )
    {
        auto &tt = tok.peek();
        if (tt.id != token_id::NIL)
        {
            if (tt.id != token_id::TRUE && tt.id != token_id::FALSE)
                tok.error(error_code::PGERR_INVALID_VALUE, "Invalid boolean value");
            value = tt.id == token_id::TRUE;
        }
        tok.next();
    }
    static void write( ostream &os, const bool &value )
    {
        os << (value ? "true" : "false");
    }
    static bool empty( const bool &value ) { (void) value; return false; }
    static void clear( bool &value ) { value = false; }
};

template<>
struct json<std::string, void>
{
    static void read( tokenizer &tok, std::string &value )
    {
        auto &tt = tok.peek();
        if (tt.id != token_id::NIL)
        {
            if (tt.id != token_id::STRING)
                tok.error(error_code::PGERR_INVALID_VALUE, "Invalid string value");
            value = tt.value;
        }
        tok.next();
    }
    static void write( ostream &os, const std::string &value )
    {
        os << '"' << value << '"';
    }
    static bool empty( const std::string &value ) { return value.empty(); }
    static void clear( std::string &value ) { value.clear(); }
};

inline bool write_members(ostream&, std::string const*, size_t, bool)
{
    return false;
}

template<typename T, typename head, typename... args>
inline bool write_members(T& os, std::string const * member_ptr,
    size_t pos, bool &first, const head& val, args& ... args_)
{
    //std::cout << "Writing " << member << " - " << pos << " -> " << member_ptr[pos] << std::endl;
    if (!json<head>::empty(val))
    {
        if (!first) os << ',';
        os << '"' << member_ptr[pos] << "\":";
        json<head>::write(os, val);
        first = false;
    }
    if (sizeof...(args))
        return write_members(os, member_ptr, pos + 1, first, args_...);
    return true;
}

inline bool read_members(tokenizer&, std::string const*, std::string const&, size_t)
{
    return false;
}

template<typename T, typename head, typename... args>
inline bool read_members(T& tok, std::string const * member_ptr,
    std::string const& member, size_t pos, head& val, args& ... args_)
{
    //std::cout << "Reading " << member << " - " << pos << " -> " << member_ptr[pos] << std::endl;
    if (member_ptr[pos] == member)
    {
        json<head>::read(tok, val);
        return true;
    }
    if (sizeof...(args))
        return read_members(tok, member_ptr, member, pos + 1, args_...);
    return false;
}

inline void clear_members(size_t) { }

template<typename head, typename... args>
inline void clear_members(size_t pos, head& val, args& ... args_)
{
    json<head>::clear(val);
    if (sizeof...(args))
        clear_members(pos + 1, args_...);
}

inline void empty_members( bool &result, size_t ) { result &= false; }

template<typename head, typename... args>
inline void empty_members(bool &result, size_t pos, const head& val, args& ... args_)
{
    result &= json<head>::empty(val);
    if (sizeof...(args))
        empty_members(result, pos + 1, args_...);
}

inline std::vector<std::string> split( char const *text )
{
    std::vector<std::string> fields;
    while (*text != 0)
    {
        while (*text == ' ' || *text == ',') ++text;
        const char *s = text;
        while (*text != ' ' && *text != ',' && *text != 0) ++text;
        if (s != text)
        {
            auto value = std::string(s, (size_t)(text-s));
            fields.push_back(value);
        }
    }
    return fields;
}

template<typename T, typename J = json<T> >
static void read_object(protogen_2_0_0::tokenizer& tok, T &object)
{
    if (!tok.expect(protogen_2_0_0::token_id::OBJS))
        tok.error(error_code::PGERR_INVALID_OBJECT, "objects must start with '{'");
    if (tok.expect(protogen_2_0_0::token_id::OBJE)) return;
    do
    {
        auto tt = tok.peek();
        if (tt.id != protogen_2_0_0::token_id::STRING)
            tok.error(error_code::PGERR_INVALID_NAME, "object key must be string");
        tok.next();
        if (!tok.expect(protogen_2_0_0::token_id::COLON))
            tok.error(error_code::PGERR_INVALID_SEPARATOR, "field name and value must be separated by ':'");
        if (!J::read_field(tok, tt.value, object))
            tok.ignore();
        if (tok.expect(protogen_2_0_0::token_id::OBJE))
            return;
        else
        if (tok.expect(protogen_2_0_0::token_id::COMMA))
            continue;
        tok.error(error_code::PGERR_INVALID_OBJECT, "invalid json object");
    } while (true);
}

#define PG_CONCAT(arg1, arg2)   PG_CONCAT1(arg1, arg2)
#define PG_CONCAT1(arg1, arg2)  arg1##arg2

#define PG_FOR_EACH_1(what, x, ...) what(x)
#define PG_FOR_EACH_2(what, x, ...) what(x) PG_FOR_EACH_1(what, __VA_ARGS__)
#define PG_FOR_EACH_3(what, x, ...) what(x) PG_FOR_EACH_2(what, __VA_ARGS__)
#define PG_FOR_EACH_4(what, x, ...) what(x) PG_FOR_EACH_3(what, __VA_ARGS__)
#define PG_FOR_EACH_5(what, x, ...) what(x) PG_FOR_EACH_4(what, __VA_ARGS__)
#define PG_FOR_EACH_6(what, x, ...) what(x) PG_FOR_EACH_5(what, __VA_ARGS__)
#define PG_FOR_EACH_7(what, x, ...) what(x) PG_FOR_EACH_6(what, __VA_ARGS__)
#define PG_FOR_EACH_8(what, x, ...) what(x) PG_FOR_EACH_7(what, __VA_ARGS__)
#define PG_FOR_EACH_9(what, x, ...) what(x) PG_FOR_EACH_8(what, __VA_ARGS__)
#define PG_FOR_EACH_10(what, x, ...) what(x) PG_FOR_EACH_9(what, __VA_ARGS__)
#define PG_FOR_EACH_11(what, x, ...) what(x) PG_FOR_EACH_10(what, __VA_ARGS__)
#define PG_FOR_EACH_12(what, x, ...) what(x) PG_FOR_EACH_11(what, __VA_ARGS__)
#define PG_FOR_EACH_13(what, x, ...) what(x) PG_FOR_EACH_12(what, __VA_ARGS__)
#define PG_FOR_EACH_14(what, x, ...) what(x) PG_FOR_EACH_13(what, __VA_ARGS__)
#define PG_FOR_EACH_15(what, x, ...) what(x) PG_FOR_EACH_14(what, __VA_ARGS__)
#define PG_FOR_EACH_16(what, x, ...) what(x) PG_FOR_EACH_15(what, __VA_ARGS__)
#define PG_FOR_EACH_17(what, x, ...) what(x) PG_FOR_EACH_16(what, __VA_ARGS__)
#define PG_FOR_EACH_18(what, x, ...) what(x) PG_FOR_EACH_17(what, __VA_ARGS__)
#define PG_FOR_EACH_19(what, x, ...) what(x) PG_FOR_EACH_18(what, __VA_ARGS__)
#define PG_FOR_EACH_20(what, x, ...) what(x) PG_FOR_EACH_19(what, __VA_ARGS__)
#define PG_FOR_EACH_21(what, x, ...) what(x) PG_FOR_EACH_20(what, __VA_ARGS__)
#define PG_FOR_EACH_22(what, x, ...) what(x) PG_FOR_EACH_21(what, __VA_ARGS__)
#define PG_FOR_EACH_23(what, x, ...) what(x) PG_FOR_EACH_22(what, __VA_ARGS__)
#define PG_FOR_EACH_24(what, x, ...) what(x) PG_FOR_EACH_23(what, __VA_ARGS__)

#define PG_FOR_EACH_NARG(...) PG_FOR_EACH_NARG_(__VA_ARGS__, PG_FOR_EACH_RSEQ_N())
#define PG_FOR_EACH_NARG_(...) PG_FOR_EACH_ARG_N(__VA_ARGS__)
#define PG_FOR_EACH_ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,N,...) N
#define PG_FOR_EACH_RSEQ_N() 24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
#define PG_FOR_EACH_(N, what, x, ...) PG_CONCAT(PG_FOR_EACH_, N)(what, x, __VA_ARGS__)
#define PG_FOR_EACH(what, x, ...) PG_FOR_EACH_(PG_FOR_EACH_NARG(x, __VA_ARGS__), what, x, __VA_ARGS__)

#define MAKE_DESERIALIZE_IF(field_name) \
    if (name == MAKE_STRING(field_name)) \
    { protogen_2_0_0::json<decltype(value.field_name)>::read(tok, value.field_name); return true; } else

#define MAKE_SERIALIZE_IF(field_name) \
    if (!protogen_2_0_0::json<decltype(value.field_name)>::empty(value.field_name)) \
    { \
        if (!first) os << ','; \
        first = false; \
        os << "\"" MAKE_STRING(field_name) "\":"; \
        protogen_2_0_0::json<decltype(value.field_name)>::write(os, value.field_name); \
    }

#define MAKE_EMPTY_IF(field_name) \
    if (!protogen_2_0_0::json<decltype(value.field_name)>::empty(value.field_name)) return false;

#define MAKE_CLEAR_CALL(field_name) \
    protogen_2_0_0::json<decltype(value.field_name)>::clear(value.field_name);

#define PG_JSON(type, ...) \
    template<> \
    struct protogen_2_0_0::json<type> \
    { \
        static void read( tokenizer &tok, type &value ) \
        { \
            read_object(tok, value); \
        } \
        static bool read_field( tokenizer &tok, const std::string &name, type &value ) \
        { \
            PG_FOR_EACH(MAKE_DESERIALIZE_IF, __VA_ARGS__); \
            return false; \
        } \
        static void write( ostream &os, const type &value ) \
        { \
            bool first = true; \
            os << '{'; \
            PG_FOR_EACH(MAKE_SERIALIZE_IF, __VA_ARGS__); \
            os << '}'; \
        } \
        static bool empty( const type &value ) \
        { \
            PG_FOR_EACH(MAKE_EMPTY_IF, __VA_ARGS__); \
            return true; \
        } \
        static void clear( type &value ) \
        { \
            PG_FOR_EACH(MAKE_CLEAR_CALL, __VA_ARGS__); \
        } \
    }; \

#define PG_JSON_ENTITY(new_type, original_type) \
    struct new_type : public original_type, \
        public protogen_2_0_0::message<original_type, protogen_2_0_0::json<original_type>> \
    { \
        using protogen_2_0_0::message<original_type, protogen_2_0_0::json<original_type>>::serialize; \
        using protogen_2_0_0::message<original_type, protogen_2_0_0::json<original_type>>::deserialize; \
        void deserialize( protogen_2_0_0::tokenizer& tok ) override \
        { \
            protogen_2_0_0::json<original_type>::read(tok, *this); \
        } \
        void serialize( protogen_2_0_0::ostream &out ) override \
        { \
            protogen_2_0_0::json<original_type>::write(out, *this); \
        } \
        void clear() override \
        { \
            protogen_2_0_0::json<original_type>::clear(*this); \
        } \
        bool empty() const override \
        { \
            return protogen_2_0_0::json<original_type>::empty(*this); \
        } \
    }; \
    template<> \
    struct protogen_2_0_0::json<new_type> \
    { \
        static void read( tokenizer &tok, new_type &value ) \
        { \
            protogen_2_0_0::json<original_type>::read(tok, value); \
        } \
        static bool read_field( tokenizer &tok, const std::string &name, original_type &value ) \
        { \
            return protogen_2_0_0::json<original_type>::read_field(tok, name, value); \
        } \
        static void write( ostream &os, const original_type &value ) \
        { \
            protogen_2_0_0::json<original_type>::write(os, value); \
        } \
        static bool empty( const original_type &value ) \
        { \
            return protogen_2_0_0::json<original_type>::empty(value); \
        } \
        static void clear( original_type &value ) \
        { \
            protogen_2_0_0::json<original_type>::clear(value); \
        } \
    };

template<typename T>
void deserialize( protogen_2_0_0::tokenizer& tok, T &value )
{
    json<T>::read(tok, value);
}

template<typename T>
void deserialize( istream &in, T &value )
{
    tokenizer tok(in);
    deserialize<T>(tok, value);
}

template<typename T>
void deserialize( const std::string &in, T &value )
{
    iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
    deserialize<T>(is, value);
}

template<typename T>
void deserialize( const std::vector<char> &in, T &value )
{
    iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
    deserialize<T>(is, value);
}

template<typename T>
void deserialize( std::istream &in, T &value )
{
    bool skip = in.flags() & std::ios_base::skipws;
    std::noskipws(in);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(in);
    iterator_istream<std::istream_iterator<char>> is(begin, end);
    try
    {
        deserialize<T>(is, value);
        if (skip) std::skipws(in);
    } catch (exception &ex)
    {
        if (skip) std::skipws(in);
        throw;
    }
}

template<typename T>
void deserialize( const char *in, size_t len, T &value )
{
    auto begin = mem_iterator<char>(in, len);
    auto end = mem_iterator<char>(in + len, 0);
    iterator_istream<mem_iterator<char>> is(begin, end);
    deserialize<T>(is, value);
}

template<typename T>
void serialize( const T &value, ostream &out )
{
    //static_assert(std::is_base_of<ostream,S>::value, "Not derived");
    json<T>::write(out, value);
}

template<typename T>
void serialize( const T &value, std::string &out )
{
    typedef std::back_insert_iterator<std::string> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T>
void serialize( const T &value, std::vector<char> &out )
{
    typedef std::back_insert_iterator<std::vector<char>> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T>
void serialize( const T &value, std::ostream &out )
{
    typedef std::ostream_iterator<char> ittype;
    ittype begin(out);
    iterator_ostream<ittype> os(begin);
    serialize<T>(value, os);
}

template<typename T, typename J = protogen_2_0_0::json<T>>
void clear( T &value ) { json<T>::clear(value); }

template<typename T, typename J = protogen_2_0_0::json<T>>
bool empty( const T &value ) { return json<T>::empty(value); }

// parent class for messages
template<typename T, typename J>
struct message
{
    typedef T underlying_type;
    typedef J serializer_type;
    virtual ~message() = default;
    virtual void deserialize( tokenizer& tok ) = 0;
    virtual void deserialize( istream &in )
    {
        tokenizer tok(in);
        deserialize(tok);
    }
    virtual void deserialize( const std::string &in )
    {
        iterator_istream<std::string::const_iterator> is(in.begin(), in.end());
        deserialize(is);
    }
    virtual void deserialize( const std::vector<char> &in )
    {
        iterator_istream<std::vector<char>::const_iterator> is(in.begin(), in.end());
        deserialize(is);
    }
    virtual void deserialize( std::istream &in )
    {
        bool skip = in.flags() & std::ios_base::skipws;
        std::noskipws(in);
        std::istream_iterator<char> end;
        std::istream_iterator<char> begin(in);
        iterator_istream<std::istream_iterator<char>> is(begin, end);
        try
        {
            deserialize(is);
            if (skip) std::skipws(in);
        } catch (exception &ex)
        {
            if (skip) std::skipws(in);
            throw;
        }
    }
    virtual void deserialize( const char *in, size_t len )
    {
        auto begin = mem_iterator<char>(in, len);
        auto end = mem_iterator<char>(in + len, 0);
        iterator_istream<mem_iterator<char>> is(begin, end);
        deserialize(is);
    }
    virtual void serialize( ostream &out ) = 0;
    virtual void serialize( std::string &out )
    {
        typedef std::back_insert_iterator<std::string> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        serialize(os);
    }
    virtual void serialize( std::vector<char> &out )
    {
        typedef std::back_insert_iterator<std::vector<char>> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        serialize(os);
    }
    virtual void serialize( std::ostream &out )
    {
        typedef std::ostream_iterator<char> ittype;
        ittype begin(out);
        iterator_ostream<ittype> os(begin);
        serialize(os);
    }
    virtual void clear() = 0;
    virtual bool empty() const  = 0;
};

} // namespace protogen_2_0_0


#endif // PROTOGEN_2_0_0
