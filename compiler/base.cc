#ifndef PROTOGEN_TEMPLATES
#define PROTOGEN_TEMPLATES


#include <iostream>
#include <string>

namespace protogen {

template<typename T> class Field
{
    protected:
        T value;
        uint32_t flags;
    public:
        Field() : flags(0) {}
        operator const T&() const { return value; }
        const T &operator()() const { return value; }
        void operator ()(const T &value ) { this->value = value; flags &= ~1; }
        bool undefined() const { return flags & 1; }
        virtual void clear() { flags |= 1; }
        bool operator==( const T &that ) { return this->value == that; }
        bool operator==( const Field<T> &that ) { return this->value == that.value; }
};

template<typename T> class NumericField : public Field<T>
{
    public:
        NumericField<T>() : Field<T>() { this->value = (T) 0; }
        NumericField<T> &operator=( const T &value ) { this->value = value; this->flags &= ~1; return *this; }
        void clear() { this->value = (T) 0; Field<T>::clear(); }
};

class StringField : public Field<std::string>
{
    public:
        StringField &operator=( const std::string &value ) { this->value = value; this->flags &= ~1; return *this; }
        void clear() { this->value.clear(); Field<std::string>::clear(); }
};

class Message {
    public:
        virtual void serialize( std::ostream &out ) const = 0;

        template<typename T>
        static void writeNumber( std::ostream &out, bool &first, const std::string &name, const T &value )
        {
            if (!first) out << ',';
            out << '"' << name << "\" : " << value;
            first = false;
        }

        static void writeString( std::ostream &out, bool &first, const std::string &name, const std::string &value )
        {
            if (!first) out << ',';
            out << '"' << name << "\" : \"" << value << '"';
            first = false;
        }

        static void writeMessage( std::ostream &out, bool &first, const std::string &name, const Message &value )
        {
            if (!first) out << ',';
            out << '"' << name << "\" : ";
            value.serialize(out);
            first = false;
        }
};

}

#endif // PROTOGEN_TEMPLATES