#ifndef PROTOGEN_PROTO3_HH
#define PROTOGEN_PROTO3_HH


#include <string>
#include <vector>
#include <iostream>


namespace protogen {


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
};


class Field
{
    public:
        FieldType type;
        std::string typeName;
        std::string name;
        int index;
        bool repeated;

        Field();
        operator std::string() const;
};


class Message
{
    public:
        std::vector<Field> fields;
        std::string name;

        operator std::string() const;
};


class Proto3
{
    public:
        std::vector<Message> messages;

        ~Proto3();

        static Proto3 *parse( std::istream &input );

        operator std::string() const;

    private:
        Proto3();
};


class exception : public std::exception
{
    public:
        int line, column;

        exception( const std::string &message, int line = 1, int column = 1 );
        ~exception();
        const char *what() const throw();
        const std::string cause() const;
    private:
        std::string message;
};


} // protogen


#endif // PROTOGEN_PROTO3_HH