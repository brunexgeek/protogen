#ifndef PROTOGEN_TEMPLATES
#define PROTOGEN_TEMPLATES


#include <iostream>
#include <string>
#include <vector>

namespace protogen {


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
};

} // namespace protogen

#endif // PROTOGEN_TEMPLATES