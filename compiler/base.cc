#ifndef PROTOGEN_TEMPLATES
#define PROTOGEN_TEMPLATES


#include <iostream>
#include <string>
#include <vector>

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
    static bool isMessage() { return false; }
    static bool isNumber() { return true; }
    static bool isString() { return false; }
    //static T defaultValue() { return 0; };
    static void clear( T &value ) { value = 0; }
};

template<> struct traits<std::string>
{
    static bool isMessage() { return false; }
    static bool isNumber() { return false; }
    static bool isString() { return true; }
    //static const std::string &defaultValue() { static std::string value; return value; };
    static void clear( std::string &value ) { value.clear(); }
};

template<typename T> class Field
{
    protected:
        T value;
        uint32_t flags;
    public:
        Field() { clear(); flags = 1; }
#if __cplusplus >= 201103L
        //explicit operator const T&() const { return value; }
#endif
        const T &operator()() const { return value; }
        //T &operator()() { return value; }
        void operator ()(const T &value ) { this->value = value; flags &= ~1; }
        bool undefined() const { return (flags & 1) != 0; }
        void clear() { traits<T>::clear(value); flags |= 1; }
        //Field<T> &operator=( const T &value ) { this->value = value; this->flags &= ~1; return *this; }
        Field<T> &operator=( const Field<T> &that ) { this->value = that.value; this->flags = that.flags; return *this; }
        bool operator==( const T &that ) const { return this->value == that; }
        bool operator==( const Field<T> &that ) const { return this->value == that.value; }
};


template<typename T> class RepeatedField
{
    protected:
        std::vector<T> value;
    public:
        RepeatedField() {}
        const std::vector<T> &operator()() const { return value; }
        std::vector<T> &operator()() { return value; }
        bool undefined() const { return value.size() == 0; }
        bool operator==( const RepeatedField<T> &that ) const { return this->value == that.value; }
};


class Message
{
    public:
        virtual void serialize( std::ostream &out ) const = 0;
        virtual void clear() = 0;
};

template <typename Iter> class InputStream {
    protected:
        Iter cur_, end_;
        int last_ch_;
        bool ungot_;
        int line_, column_;
    public:
        InputStream( const Iter& first, const Iter& last ) : cur_(first), end_(last),
            last_ch_(-1), ungot_(false), line_(1), column_(1)
        {
        }

        int get()
        {
            if (ungot_)
            {
                ungot_ = false;
                return last_ch_;
            }

            if (cur_ == end_)
            {
                last_ch_ = -1;
                return -1;
            }
            if (last_ch_ == '\n')
            {
                line_++;
                column_ = 1;
            }
            last_ch_ = *cur_ & 0xff;
            ++cur_;
            ++column_;
            return last_ch_;
        }

        void unget()
        {
            if (last_ch_ != -1)
            {
                if (ungot_) throw std::runtime_error("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const { return *cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skip_ws()
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
            skip_ws();
            if (get() != expect)
            {
                unget();
                return false;
            }
            return true;
        }

        bool match(const std::string& pattern)
        {
            for (std::string::const_iterator pi(pattern.begin()); pi != pattern.end(); ++pi)
            {
                if (get() != *pi)
                {
                    unget();
                    return false;
                }
            }
            return true;
        }
};


class JSON
{
    public:
        template<typename T>
        static void write( std::ostream &out, bool &first, const std::string &name, const T &value )
        {
            if (!first) out << ',';
            out << '"' << name << "\" : " << value;
            first = false;
        }

        static void write( std::ostream &out, bool &first, const std::string &name, const std::string &value)
        {
            if (!first) out << ',';
            out << '"' << name << "\" : \"" << value << '"';
            first = false;
        }

        static void write( std::ostream &out, bool &first, const std::string &name, const Message &value)
        {
            if (!first) out << ',';
            out << '"' << name << "\" : ";
            value.serialize(out);
            first = false;
        }

        /*template<typename I>
        static void skipws( InputStream<I> &in )
        {
            while (1)
            {
                int ch = in.get();
                if (! (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'))
                {
                    in.unget();
                    break;
                }
            }
        }*/

        template<typename I>
        static bool next( InputStream<I> &in )
        {
            in.skip_ws();
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
        static bool read( InputStream<I> &in, std::string &value )
        {
            in.skip_ws();
            if (in.get() != '"') return false;
            int c = 0;
            while (1)
            {
                int c = in.get();
                if (c == '"') return true;
                if (c <= 0) return false;
                value += (char) c;
            }

            return false;
        }

        template<typename I, typename T>
        static bool read( InputStream<I> &in, T &value )
        {
            in.skip_ws();
            //in >> value;
            //return !in.fail();
            return 10;
        }

        template<typename I, typename T>
        static bool readArray( InputStream<I> &in, std::vector<T> &array )
        {
            in.skip_ws();
            while (true)
            {
                T value;
                if (!read(in, value)) return false;
                array.push_back(value);
            }

            return 10;
        }

        template<typename I>
        static bool readName( InputStream<I> &in, std::string &name )
        {
            if (!read(in, name)) return false;
            in.skip_ws();
            return (in.get() == ':');
        }
};

} // namespace protogen

#endif // PROTOGEN_TEMPLATES