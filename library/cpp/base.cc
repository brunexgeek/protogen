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
        uint32_t flags_;
    public:
        Field() { clear(); flags_ = 1; }
#if __cplusplus >= 201103L
        Field( T &&that ) { that.value_ = this->value_; that.flags_ = this->flags_; }
#endif
        const T &operator()() const { return value_; }
        T &operator()() { return value_; }
        void operator ()(const T &value ) { this->value_ = value; this->flags_ &= ~1; }
        bool undefined() const { return (flags_ & 1) != 0; } // TODO: use traits for 'undefined' (at least with messages)
        void clear() { traits<T>::clear(value_); flags_ |= 1; }
        Field<T> &operator=( const Field<T> &that ) { this->value_ = that.value_; this->flags_ = that.flags_; return *this; }
        bool operator==( const T &that ) const { return this->value_ == that && (this->flags_ & 1) == 0; }
        bool operator==( const Field<T> &that ) const { return this->value_ == that.value_ && this->flags_ == that.flags_; }
};

template<typename T> class RepeatedField
{
    protected:
        std::vector<T> value_;
        int number_;
    public:
        RepeatedField() {}
#if __cplusplus >= 201103L
        RepeatedField( T &&that ) { that.value_ = std::move(this->value_); }
#endif
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
        virtual bool deserialize( std::istream &in ) = 0;
        virtual bool deserialize( std::string &in ) = 0;
        virtual void clear() = 0;
};

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

template<typename I>
static bool readName( InputStream<I> &in, std::string &name )
{
    if (!readValue(in, name)) return false;
    in.skipws();
    return (in.get() == ':');
}

template<typename I>
static bool ignoreEnclosing( InputStream<I> &in, int begin, int end )
{
    if (in.get() != begin) return false;

    int count = 1;

    bool text = false;
    bool escape = false;
    while (count != 0 && !in.eof())
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
        if (ch == '\\')
        {
            if (!text) return false; // escape sequence not allowed outside strings
            escape = true;
        }
        else
        if (ch == '"' && !escape)
            text = !text;
        else
        if (!text)
        {
            if (ch == end)
                --count;
            else
            if (ch == begin)
                ++count;
        }
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
        return ignoreEnclosing(in, '"', '"');
    else
    {
        while (!in.eof())
        {
            int ch = in.get();
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == ',')
            {
                in.unget();
                return true;
            }
        }
        in.skipws();
        if (in.get() != ',') in.unget();
    }

    return false;
}

} // namespace json
} // namespace protogen

#endif // PROTOGEN_BASE