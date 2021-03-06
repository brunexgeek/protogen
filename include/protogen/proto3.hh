/*
 * Copyright 2020 Bruno Ribeiro <https://github.com/brunexgeek>
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

#ifndef PROTOGEN_PROTO3_HH
#define PROTOGEN_PROTO3_HH


#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <protogen/exception.hh>


namespace protogen {


#ifndef PROTOGEN_FIELD_TYPES
#define PROTOGEN_FIELD_TYPES

enum FieldType
{
    TYPE_DOUBLE   = 6,
    TYPE_FLOAT    = 7,
    TYPE_INT32    = 8,
    TYPE_INT64    = 9,
    TYPE_UINT32   = 10,
    TYPE_UINT64   = 11,
    TYPE_SINT32   = 12,
    TYPE_SINT64   = 13,
    TYPE_FIXED32  = 14,
    TYPE_FIXED64  = 15,
    TYPE_SFIXED32 = 16,
    TYPE_SFIXED64 = 17,
    TYPE_BOOL     = 18,
    TYPE_STRING   = 19,
    TYPE_BYTES    = 20,
    TYPE_MESSAGE  = 21,
};

#endif // PROTOGEN_FIELD_TYPES


class Message;

struct TypeInfo
{
    FieldType id;
    std::string qname;
    Message *ref;
    bool repeated;

    TypeInfo();
};


enum class OptionType
{
    IDENTIFIER,
    STRING,
    INTEGER,
    BOOLEAN
};


struct OptionEntry
{
    std::string name;
    OptionType type;
    std::string value;
    int line;
};


typedef std::unordered_map<std::string, OptionEntry> OptionMap;


class Field
{
    public:
        TypeInfo type;
        std::string name;
        int index;
        OptionMap options;

        Field();
};


class Message
{
    public:
        std::vector<Field> fields;
        std::string name;
        std::string package;
        OptionMap options;

        std::string qualifiedName() const;
        void splitPackage( std::vector<std::string> &out );
};


class Proto3
{
    public:
        std::vector<Message*> messages;
        OptionMap options;
        std::string fileName;

        ~Proto3();
        static void parse( Proto3 &tree, std::istream &input, const std::string &fileName = "");
};


} // protogen

std::ostream &operator<<( std::ostream &out, protogen::Proto3 &proto );
std::ostream &operator<<( std::ostream &out, protogen::Message &message );
std::ostream &operator<<( std::ostream &out, protogen::Field &field );


#endif // PROTOGEN_PROTO3_HH