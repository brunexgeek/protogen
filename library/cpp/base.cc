#ifndef PROTOGEN_BASE
#define PROTOGEN_BASE

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <locale.h>
#if __cplusplus >= 201103L
    #include <algorithm>
#endif

namespace protogen {

#ifndef PROTOGEN_FIELD_TYPES
#define PROTOGEN_FIELD_TYPES

enum FieldType
{
    TYPE_DOUBLE       =  6,
    TYPE_FLOAT        =  7,
    TYPE_INT32        =  8,
    TYPE_INT64        =  9,
    TYPE_UINT32       =  10,
    TYPE_UINT64       =  11,
    TYPE_SINT32       =  12,
    TYPE_SINT64       =  13,
    TYPE_FIXED32      =  14,
    TYPE_FIXED64      =  15,
    TYPE_SFIXED32     =  16,
    TYPE_SFIXED64     =  17,
    TYPE_BOOL         =  18,
    TYPE_STRING       =  19,
    TYPE_BYTES        =  20,
    TYPE_MESSAGE      =  21,
};

#endif // PROTOGEN_FIELD_TYPES

template<typename T> struct traits
{
    static void clear( T &value ) { value = (T) 0; }
    static void write( std::ostream &out, const T &value ) { out << value; }
};

template<> struct traits<bool>
{
    static void clear( bool &value ) { value = false; }
    static void write( std::ostream &out, const bool &value ) { out << ((value) ? "true" : "false"); }
};

template<> struct traits<uint8_t>
{
    static void clear( uint8_t &value ) { value = 0; }
    static void write( std::ostream &out, const uint8_t &value ) { out << (int) value; }
};

template<> struct traits<std::string>
{
    static void clear( std::string &value ) { value.clear(); }
    static void write( std::ostream &out, const std::string &value )
    {
        out << '"';
        for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
        {
            switch (*it)
            {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '/':  out << "\\/"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\r': out << "\\r"; break;
                case '\n': out << "\\n"; break;
                case '\t': out << "\\t"; break;
                default:   out << *it;
            }
        }
        out << '"';
    }
};

template<typename T> class Field
{
    protected:
        T value_;
        bool undefined_;
    public:
        Field() { clear(); }
        const T &operator()() const { return value_; }
        void operator ()(const T &value ) { this->value_ = value; this->undefined_ = false; }
        bool undefined() const { return undefined_; }
        void clear() { traits<T>::clear(value_); undefined_ = true; }
        Field<T> &operator=( const Field<T> &that ) { this->value_ = that.value_; this->undefined_ = that.undefined_; return *this; }
        bool operator==( const T &that ) const { return !this->undefined_ && this->value_ == that; }
        bool operator==( const Field<T> &that ) const { return this->undefined_ == that.undefined_ && this->value_ == that.value_;  }
};

template<typename T> class RepeatedField
{
    protected:
        std::vector<T> value_;
        int number_;
    public:
        RepeatedField() {}
        const std::vector<T> &operator()() const { return value_; }
        std::vector<T> &operator()() { return value_; }
        bool undefined() const { return value_.size() == 0; }
        void clear() { value_.clear(); }
        bool operator==( const RepeatedField<T> &that ) const { return this->value_ == that.value_; }
};

class Message
{
    public:
        virtual void serialize( std::string &out ) const = 0;
        virtual void serialize( std::ostream &out ) const = 0;
        virtual bool deserialize( std::istream &in, bool required = false ) = 0;
        virtual bool deserialize( const std::string &in, bool required = false ) = 0;
        virtual bool deserialize( const std::vector<char> &in, bool required = false ) = 0;
        virtual void clear() = 0;
        virtual bool undefined() const = 0;
};

#if 0
class MemoryBuffer : public std::basic_streambuf<uint8_t>
{
    public:
		MemoryBuffer( uint8_t* data, size_t size ) : data_(data), size_(size), ptr_(data) {}
        uint8_t* pbase() const { return data_; }
		uint8_t* pptr() const { return ptr_; }
		uint8_t* epptr() const { return data_ + size_; }

    private:
        uint8_t *data_;
        size_t size_;
        uint8_t *ptr_;
};
#endif


template <typename I> class InputStream
{
    protected:
        I cur_, end_;
        int last_, line_, column_;
        bool ungot_;

    public:
        InputStream( const I& first, const I& last ) : cur_(first), end_(last),
            last_(-1), line_(1), column_(1), ungot_(false)
        {
        }

        bool eof()
        {
            return last_ == -2;
        }

        int get()
        {
            if (ungot_)
            {
                ungot_ = false;
                return last_;
            }
            if (last_ == '\n')
            {
                ++line_;
                column_ = 0;
            }
            if (cur_ == end_)
            {
                last_ = -2;
                return -1;
            }

            last_ = *cur_ & 0xFF;
            ++cur_;
            ++column_;
            return last_;
        }

        void unget()
        {
            if (last_ >= 0)
            {
                if (ungot_) throw std::runtime_error("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const { return *cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skipws()
        {
            while (1) {
                int ch = get();
                if (! (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'))
                {
                    unget();
                    break;
                }
            }
        }

        bool expect(int expect)
        {
            if (get() != expect)
            {
                unget();
                return false;
            }
            return true;
        }
};

namespace json {


static std::string reveal( const char *value, size_t length )
{
	std::string result(length, ' ');
	for (size_t i = 0; i < length; ++i)
		result[i] = (char) ((int) value[i] ^ 0x33);
	return result;
}

// numeric types
template<typename T>
static bool write( std::ostream &out, bool &first, const std::string &name, const T &value )
{
    if (!first) out << ',';
    out << '"' << name << "\":";
    traits<T>::write(out, value);
    first = false;
    return true;
}

template<typename T>
static bool write( std::ostream &out, bool &first, const std::string &name, const std::vector<T> &value )
{
    if (!first) out << ",";

    bool begin = true;
    out << '"' << name << "\":[";
    for (typename std::vector<T>::const_iterator it = value.begin(); it != value.end(); ++it)
    {
        if (!begin) out << ",";
        begin = false;
        traits<T>::write(out, *it);
    }
    out << "]";

    first = false;
    return true;
}

// Base64 encoder/decoder based on Joe DF's implementation
// Original source at <https://github.com/joedf/base64.c> (MIT licensed)
static char B64_SYMBOLS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64_int( int ch )
{
	if (ch == '+') return 62;
	if (ch == '/') return 63;
	if (ch == '=') return 64;
	if (ch >= '0' && ch <= '9') return ch + 4;
	if (ch >= 'A' && ch <= 'Z') return ch - 'A';
	if (ch >= 'a' && ch <= 'z') return (ch - 'a') + 26;
	return 0;
}

template<>
bool write( std::ostream &out, bool &first, const std::string &name, const std::vector<uint8_t> &value )
{
    char o[5] = { 0 };
    size_t i = 0;
    size_t size = value.size();

    if (!first) out << ',';
    first = true;

    out << '"' << name << "\":\"";

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

    return true;
}


template<typename I>
static bool next( InputStream<I> &in )
{
    in.skipws();
    int ch = in.get();
    if (ch == ',') return true;
    if (ch == '}' || ch == ']')
    {
        in.unget();
        return true;
    }
    return false;
}

template<typename I>
static bool readValue( InputStream<I> &in, std::string &value )
{
    in.skipws();
    if (in.get() != '"') return false;
    while (1)
    {
        int c = in.get();
        if (c == '"') return true;

        if (c == '\\')
        {
            c = in.get();
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
            }
        }

        if (c <= 0) return false;
        value += (char) c;
    }

    return false;
}

template<typename I, typename T>
static bool readValue( InputStream<I> &in, T &value )
{
    in.skipws();
    std::string temp;
    while (true)
    {
        int ch = in.get();
        if (ch != '.' && (ch < '0' || ch > '9'))
        {
            in.unget();
            break;
        }
        temp += (char) ch;
    }
    if (temp.empty()) return false;

    static locale_t loc = newlocale(LC_NUMERIC_MASK | LC_MONETARY_MASK, "C", 0);
    if (loc == 0) return false;
    uselocale(loc);
    value = (T) strtod(temp.c_str(), NULL);
    uselocale(LC_GLOBAL_LOCALE);
    return true;
}

template<typename I>
static bool readValue( InputStream<I> &in, bool &value )
{
    in.skipws();
    std::string temp;
    while (true)
    {
        int ch = in.get();
        switch (ch)
        {
            case 't':
            case 'r':
            case 'u':
            case 'e':
            case 'f':
            case 'a':
            case 'l':
            case 's':
                temp += (int) ch;
                break;
            default:
                in.unget();
                goto ESCAPE;
        }
    }
ESCAPE:
    if (temp == "true")
        value = true;
    else
    if (temp == "false")
        value = false;
    else
        return false;
    return true;
}

// proto3 'repeated' message
template<typename I, typename T>
static bool readMessageArray( InputStream<I> &in, std::vector<T> &array )
{
    in.skipws();
    if (in.get() != '[') return false;
    in.skipws();
    while (true)
    {
        T value;
        value.deserialize(in);
        array.push_back(value);
        in.skipws();
        if (!in.expect(',')) break;
    }
    if (in.get() != ']') return false;

    return true;
}

// proto3 'repeated' primitives
template<typename I, typename T>
static bool readArray( InputStream<I> &in, std::vector<T> &array )
{
    in.skipws();
    if (in.get() != '[') return false;
    in.skipws();
    while (true)
    {
        T value;
        if (!readValue(in, value)) return false;
        array.push_back(value);
        in.skipws();
        if (!in.expect(',')) break;
    }
    if (in.get() != ']') return false;

    return true;
}

// proto3 'bytes'
template<typename I>
static bool readArray( InputStream<I> &in, std::vector<uint8_t> &array )
{
	size_t k = 0, j;
    int s[4];

    if (in.get() != '"') return false;

	while (true)
    {
        // read 4 characters
        for (j = 0; j < 4; ++j)
        {
            int ch = in.get();
            if (ch == '"') return j == 0;
		    s[j] = b64_int(ch);
        }
        // decode base64 tuple
		array.push_back( ((s[0] & 0xFF) << 2 ) | ((s[1] & 0x30) >> 4) );
        if (s[2] != 64)
        {
            array.push_back( ((s[1] & 0x0F) << 4) | ((s[2] & 0x3C) >> 2) );
            if ((s[3]!=64))
            {
                array.push_back( ((s[2] & 0x03) << 6) | s[3] );
                k+=3;
            }
            else
                k+=2;
        }
        else
            k+=1;
	}
}

template<typename I>
static bool readName( InputStream<I> &in, std::string &name )
{
    if (!readValue(in, name)) return false;
    in.skipws();
    return (in.get() == ':');
}


template<typename I>
static bool ignoreString( InputStream<I> &in )
{
    if (in.get() != '"') return false;

    bool escape = false;
    while (!in.eof())
    {
        int ch = in.get();
        if (escape)
        {
            switch (ch)
            {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'f':
                case 'r':
                case 'n':
                case 't':
                    escape = false;
                    break;
                default: // invalid escape sequence
                    return false;
            }
        }
        else
        if (ch == '"')
            return true;
    }

    return false;
}


template<typename I>
static bool ignoreEnclosing( InputStream<I> &in, int begin, int end )
{
    if (in.get() != begin) return false;

    int count = 1;

    bool text = false;
    while (count != 0 && !in.eof())
    {
        int ch = in.get();
        if (ch == '"' && !text)
        {
            in.unget();
            if (!ignoreString(in)) return false;
        }
        else
        if (ch == end)
            --count;
        else
        if (ch == begin)
            ++count;
    }

    in.skipws();
    if (in.get() != ',') in.unget();

    return count == 0;
}

template<typename I>
static bool ignore( InputStream<I> &in )
{
    in.skipws();
    int ch = in.get();
    in.unget();
    if (ch == '[')
        return ignoreEnclosing(in, '[', ']');
    else
    if (ch == '{')
        return ignoreEnclosing(in, '{', '}');
    else
    if (ch == '"')
        return ignoreString(in);
    else
    {
        while (!in.eof())
        {
            int ch = in.get();
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == ',' || ch == ']' || ch == '}')
            {
                in.unget();
                return true;
            }
        }
    }

    return false;
}

} // namespace json
} // namespace protogen

#endif // PROTOGEN_BASE