#ifndef PROTOGEN_PROTO3_HH
#define PROTOGEN_PROTO3_HH


#include <string>
#include <vector>
#include <iostream>


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
        std::string package;

        operator std::string() const;
};


class Proto3
{
    public:
        std::vector<Message> messages;

        Proto3();

        ~Proto3();

        static void parse( std::istream &input, Proto3 &tree );

        operator std::string() const;

    private:

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

std::ostream &operator<<( std::ostream &out, protogen::Proto3 &proto );
std::ostream &operator<<( std::ostream &out, protogen::Message &message );
std::ostream &operator<<( std::ostream &out, protogen::Field &field );


#endif // PROTOGEN_PROTO3_HH