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

        static void skipws( std::istream &in )
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
        }

        static bool readString( std::istream &in, std::string &value )
        {
            skipws(in);
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

        template<typename T>
        static bool readNumber( std::istream &in, T &value )
        {
            skipws(in);
            in >> value;
            return !in.fail();
        }

        static bool readName( std::istream &in, std::string &name )
        {
            if (!readString(in, name)) return false;
            skipws(in);
            return (in.get() == ':');
        }
};

} // namespace protogen

#endif // PROTOGEN_TEMPLATES