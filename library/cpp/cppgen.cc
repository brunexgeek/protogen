/*
 * Copyright 2018 Bruno Ribeiro <https://github.com/brunexgeek>
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

#include <protogen/protogen.hh>
#include "../printer.hh"


extern const char *BASE_TEMPLATE;


namespace protogen {


#define IS_VALID_TYPE(x)      ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )
#define IS_NUMERIC_TYPE(x)    ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_SFIXED64 )


static struct {
    bool needTemplate;
    protogen::FieldType type;
    const char *typeName;
    const char *nativeType;
    const char *defaultValue;
} TYPE_MAPPING[] =
{
    { true,  protogen::TYPE_DOUBLE,   "double",    "double"      , "0.0" },
    { true,  protogen::TYPE_FLOAT,    "float",     "float"       , "0.0F" },
    { true,  protogen::TYPE_INT32,    "int32",     "int32_t"     , "0" },
    { true,  protogen::TYPE_INT64,    "int64",     "int64_t"     , "0" },
    { true,  protogen::TYPE_UINT32,   "uint32",    "uint32_t"    , "0" },
    { true,  protogen::TYPE_UINT64,   "uint64",    "uint64_t"    , "0" },
    { false, protogen::TYPE_SINT32,   "sint32",    "int32_t"     , "0" },
    { false, protogen::TYPE_SINT64,   "sint64",    "int64_t"     , "0" },
    { false, protogen::TYPE_FIXED32,  "fixed32",   "uint32_t"    , "0" },
    { false, protogen::TYPE_FIXED64,  "fixed64",   "uint64_t"    , "0" },
    { false, protogen::TYPE_SFIXED32, "sfixed32",  "int32_t"     , "0" },
    { false, protogen::TYPE_SFIXED64, "sfixed64",  "int64_t"     , "0" },
    { true,  protogen::TYPE_BOOL,     "bool",      "bool"        , "false" },
    { true,  protogen::TYPE_STRING,   "string",    "std::string" , "\"\"" },
    { true,  protogen::TYPE_BYTES,    "bytes",     "uint8_t"     , nullptr },
    { false, protogen::TYPE_MESSAGE,  nullptr,     nullptr       , nullptr }
};


#if 0

static std::string toLower( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'A' && output[i] <= 'Z') output[i] = (char)(output[i] + 32);

    return output;
}
#endif

static std::string toUpper( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'a' && output[i] <= 'z') output[i] = (char)(output[i] - 32);

    return output;
}


static std::string obfuscate( const std::string &value )
{
    size_t len = value.length();
	std::string result;
	for (size_t i = 0; i < len; ++i)
    {
        int cur = value[i] ^ 0x33;
        static const char *ALPHABET = "0123456789ABCDEF";
        result += "\\x";
        result += ALPHABET[ (cur & 0xF0) >> 4 ];
        result += ALPHABET[ cur & 0x0F ];
    }
	return result;
}


static std::string nativePackage( const std::vector<std::string> &package )
{
    // extra space because the compiler may complain about '<::' (i.e. using in templates)
    if (package.empty() || package[0].empty()) return "";

    std::string name = " ::";
    for (auto it = package.begin(); it != package.end(); ++it)
    {
        name += *it;
        if (it + 1 != package.end()) name += "::";
    }
    return name;
}


static std::string fieldStorage( const Field &field )
{
    return field.name;
}


/**
 * Translates protobuf3 types to C++ types.
 */
static std::string nativeType( const Field &field )
{

    if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_STRING)
    {
        int index = (int)field.type.id - (int)protogen::TYPE_DOUBLE;
        return TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type.id == protogen::TYPE_BYTES)
        return "uint8_t";
    else
    if (field.type.id == protogen::TYPE_MESSAGE)
        return field.type.name;
    else
        throw protogen::exception("Invalid field type");
}

/**
 * Translates protobuf3 types to protogen types.
 */
static std::string fieldNativeType( const Field &field )
{
    std::string output = "protogen::";

    if (field.repeated || field.type.id == protogen::TYPE_BYTES)
        output += "RepeatedField<";
    else
        output += "Field<";

    if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_BYTES)
    {
        int index = (int)field.type.id - (int)protogen::TYPE_DOUBLE;
        output += TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type.id == protogen::TYPE_MESSAGE)
        output += field.type.name;
    else
        throw protogen::exception("Invalid field type");

    output += '>';
    return output;
}


static void generateVariable( Printer &printer, const Field &field )
{
    std::string storage = fieldStorage(field);

    char number[10];
    snprintf(number, sizeof(number), "%d", field.index);
    number[9] = 0;

    printer(
        "static const int $3$_NO = $4$;\n"
        "$1$ $2$;\n", fieldNativeType(field), storage, toUpper(storage), number);
}


static void generateCopyCtor( Printer &printer, const Message &message )
{
    printer("$1$(const $1$ &that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
        printer("this->$1$ = that.$1$;\n", fieldStorage(*fi));
    printer("\b}\n");
}


static void generateMoveCtor( Printer &printer, const Message &message )
{
    printer(
        "#if __cplusplus >= 201103L\n"
        "$1$($1$ &&that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        printer("this->$1$ = std::move(that.$1$);\n", fieldStorage(*fi));
    }
    printer("\b}\n#endif\n");
}


static void generateAssignOperator( Printer &printer, const Message &message )
{
    printer("$1$ &operator=(const $1$ &that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
        printer("this->$1$ = that.$1$;\n", fieldStorage(*fi));
    printer("return *this;\n\b}\n");
}


static void generateEqualityOperator( Printer &printer, const Message &message )
{
    printer(
        "bool operator==(const $1$&that) const {\n"
        "\treturn\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        printer("this->$1$ == that.$1$", fieldStorage(*fi));
        if (fi + 1 != message.fields.end())
            printer(" &&\n");
        else
            printer(";\n");
    }
    printer("\b\b}\n");
}


static void generateDeserializer( Printer &printer, const Message &message )
{
    printer(
        // deserializer receiving a 'istream'
        "bool deserialize( std::istream &in, bool required = false ) {\n"
        "\tbool skip = in.flags() & std::ios_base::skipws;\n"
        "std::noskipws(in);\n"
        "std::istream_iterator<char> itb(in);\n"
        "std::istream_iterator<char> ite;\n"
        "protogen::InputStream< std::istream_iterator<char> > is(itb, ite);\n"
        "bool result = this->deserialize(is, required);\n"
        "if (skip) std::skipws(in);\n"
        "\treturn result;\n"
        "\b\b}\n"

        // deserializer receiving a 'string'
        "bool deserialize( const std::string &in, bool required = false ) {\n"
        "\tprotogen::InputStream<std::string::const_iterator> is(in.begin(), in.end());\n"
        "return this->deserialize(is, required);\n"
        "\b}\n"

        // deserializer receiving a 'vector'
        "bool deserialize( const std::vector<char> &in, bool required = false ) {\n"
        "\tprotogen::InputStream<std::vector<char>::const_iterator> is(in.begin(), in.end());\n"
        "return this->deserialize(is, required);\n"
        "\b}\n"

        // deserializer receiving iterators
       	"template <typename IT>\n"
        "bool deserialize( IT begin, IT end, bool required = false ){\n"
        "\tprotogen::InputStream<IT> is(begin, end);\n"
        "return this->deserialize(is, required);\n\b}\n"

        // 'real' deserializer
        "template<typename T>\n"
        "bool deserialize( protogen::InputStream<T> &in, bool required = false ) {\n"
        "\tin.skipws();\n"
        "if (in.get() != '{') return false;\n"
        "std::string name;\n");

    printer(
        "bool hasField[$1$] = {false};\n"
        "while (true) {\n\t"
        "name.clear();\n"
        "if (!protogen::json::readName(in, name)) break;\n", message.fields.size());

    bool first = true;
    size_t count = 0;

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi, ++count)
    {
        if (!IS_VALID_TYPE(fi->type.id)) continue;

        std::string storage = fieldStorage(*fi);

        if (!first) printer("else\n");
        printer("// $1$\n", fi->name);
        printer("if (name == PROTOGEN_FN_$1$) {\n\t", storage); // open the main 'if'

        if (fi->repeated || fi->type.id == protogen::TYPE_BYTES)
        {
            std::string function = "readArray";
            if (fi->type.id == protogen::TYPE_MESSAGE) function = "readMessageArray";
            printer("if (!protogen::json::$1$(in, this->$2$())) return false;\n", function, storage);
        }
        else
        {
            if (fi->type.id >= protogen::TYPE_DOUBLE && fi->type.id <= protogen::TYPE_STRING)
            {
                printer(
                    "$1$ value;\n"
                    "if (!protogen::json::readValue(in, value)) return false;\n"
                    "this->$2$(value);\n", nativeType(*fi), storage);
            }
            else
            if (fi->type.id == protogen::TYPE_MESSAGE)
                printer("if (!this->$1$().deserialize(in, required)) return false;\n", storage);
        }
        printer(
            "if (!protogen::json::next(in)) return false;\n"
            "hasField[$1$] = true;\n"
            "\b}\n", // closes the main 'if'
            count);
        first = false;
    }
    printer(
        "else\n"
        "// ignore the current field\n"
        "\tif (!protogen::json::ignore(in)) return false;\n\b\b}\n");

    printer(
        "if (required) {\n"
        "\tfor (size_t i = 0; i < $1$; ++i) if (!hasField[i]) return false;\n\b}\n"
        "return true;\n\b}\n", message.fields.size());
}


static void generateSerializer( Printer &printer, const Message &message )
{
    printer(
        // serializer writing to 'string'
        "void serialize( std::string &out ) const {\n"
        "\tstd::stringstream ss;\n"
        "serialize(ss);\n"
        "out = ss.str();\n\b}\n"

        // serializer writing to 'vector'
        "void serialize( std::vector<char> &out ) const {\n"
        "\tstd::string temp;\n"
        "serialize(temp);\n"
        "out.resize(temp.length());\n"
        "memcpy(&out.front(), temp.c_str(), temp.length());\n\b}\n"

        // serializer writing to 'ostream'
        "void serialize( std::ostream &out ) const {\n"
        "\tout << '{';\n"
        "bool first = true;\n");

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);

        printer("// $1$\n", storage);

        if (fi->type.id != protogen::TYPE_MESSAGE)
            printer("if (!this->$1$.undefined()) ", storage);

        printer("protogen::json::write(out, first, PROTOGEN_FN_$1$, this->$1$());\n", storage);
    }
    printer("out << '}';\n\b}\n");
}


static void generateTraitMacro( Printer &printer )
{
    printer(
        "\n#define PROTOGEN_TRAIT_MACRO(MSGTYPE) \\\n"
        "\tnamespace protogen { \\\n"
        "\ttemplate<> struct traits<MSGTYPE> { \\\n"
        "\tstatic void clear( MSGTYPE &value ) { value.clear(); } \\\n"
        "\tstatic void write( std::ostream &out, const MSGTYPE &value ) { value.serialize(out); } \\\n"
        "\b}; \\\n\b} \n\b\b");
}


static void generateTrait( Printer &printer, const Message &message )
{
    printer(
        "PROTOGEN_TRAIT_MACRO($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateClear( Printer &printer, const Message &message )
{
    printer("void clear() {\n\t");
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        printer("this->$1$.clear();\n", fieldStorage(*fi));
    }
    printer("\b}\n");
}


static void generateNamespace( Printer &printer, const Message &message, bool start )
{
    if (message.package.empty() || message.package[0].empty()) return;

    for (auto it = message.package.begin(); it != message.package.end(); ++it)
    {
        if (start)
            printer("namespace $1$ {\n", *it);
        else
            printer("} // namespace $1$\n", *it);
    }
}



static void generateWriterPrototypeMacro( Printer &printer )
{
    printer(
        "\n#define PROTOGEN_WRITER_PROTO_TEMPLATE(MSGTYPE) \\\n"
        "\tnamespace protogen { \\\n"
        "\tstatic void write( std::ostream &out, bool &first, const std::string &name, \\\n"
        "\tconst MSGTYPE &value); \\\n\b\b"
        "}\n\b");
}



static void generateWriterPrototype( Printer &printer, const Message &message )
{
    printer("PROTOGEN_WRITER_PROTO_TEMPLATE($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateWriterMacro( Printer &printer )
{
    printer(
        "\n#define PROTOGEN_WRITER_TEMPLATE(MSGTYPE) \\\n"
        "\tnamespace protogen { \\\n"
        "namespace json { \\\n\t"
        "static void write( std::ostream &out, bool &first, const std::string &name, const MSGTYPE &value) { \\\n"
        "\tif (!first) out << ',';\\\n"
        "out << '\"' << name << \"\\\":\"; \\\n"
        "value.serialize(out); \\\n"
        "first = false; \\\n\b} \\\n\b"
        "}}\n\b");
}


static void generateWriter( Printer &printer, const Message &message )
{
    printer("PROTOGEN_WRITER_TEMPLATE($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateFieldTemplateMacro( Printer &printer )
{
    printer(
        "\n#define PROTOGEN_FIELD_TEMPLATE(MSGTYPE) \\\n"
        "\tnamespace protogen { \\\n"
        "template<> class Field<MSGTYPE> { \\\n"
        "\tprotected: \\\n"
        "\tMSGTYPE value_; \\\n"
        "\bpublic: \\\n"
        "\tField() { clear(); } \\\n"
        "const MSGTYPE &operator()() const { return value_; } \\\n"
        "MSGTYPE &operator()() { return value_; } \\\n"
        "void operator ()(const MSGTYPE &value ) { this->value_ = value; } \\\n"
        "bool undefined() const { return value_.undefined(); } \\\n"
        "void clear() { traits<MSGTYPE>::clear(value_); } \\\n"
        "Field<MSGTYPE> &operator=( const Field<MSGTYPE> &that ) { this->value_ = that.value_; return *this; } \\\n"
        "bool operator==( const MSGTYPE &that ) const { return this->value_ == that; } \\\n"
        "bool operator==( const Field<MSGTYPE> &that ) const { return this->value_ == that.value_; } \\\n"
        "\b\b}; \\\n}\n\b");
}


static void generateFieldTemplate( Printer &printer, const Message &message )
{
    printer(
        "PROTOGEN_FIELD_TEMPLATE($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateUndefined( Printer &printer, const Message &message )
{
    printer(
        "bool undefined() const {\n"
        "\treturn\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        printer("this->$1$.undefined()", fieldStorage(*fi));
        if (fi + 1 != message.fields.end())
            printer(" &&\n");
        else
            printer(";\n");
    }
    printer("\b\b}\n");
}


static void generateMessage( Printer &printer, const Message &message )
{
    printer("\n//\n// $1$\n//\n", message.name);

    // JSON writer
    generateWriterPrototype(printer, message);

    // begin namespace
    generateNamespace(printer, message, true);

    printer(
        "\nclass $1$ : public protogen::Message {\npublic:\n\t", message.name);

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);

        printer(
            "#undef PROTOGEN_FN_$1$\n"
            "#ifdef PROTOGEN_OBFUSCATE_NAMES\n"
            "\t#define PROTOGEN_FN_$1$ protogen::json::reveal(\"$2$\", $3$)\n"
            "\b#else\n"
            "\t#define PROTOGEN_FN_$1$ \"$1$\"\n"
            "\b#endif\n",
                storage, obfuscate(storage), storage.length());
    }

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        // storage variable
        generateVariable(printer, *fi);
    }

    // default constructor and destructor
    printer("$1$() {}\nvirtual ~$1$() {}\n", message.name);
    // copy constructor
    generateCopyCtor(printer, message);
    // move constructor
    //generateMoveCtor(printer, message);
    // assign operator
    generateAssignOperator(printer, message);
    // equality operator
    generateEqualityOperator(printer, message);
    // undefined function
    generateUndefined(printer, message);
    // message serializer
    generateSerializer(printer, message);
    // message deserializer
    generateDeserializer(printer, message);
    // clear function
    generateClear(printer, message);
    // close class declaration
    printer("\b};\n");
    // end namespace
    generateNamespace(printer, message, false);

    // message trait
    generateTrait(printer, message);
    // JSON writer
    generateWriter(printer, message);
    // 'Field' template specializarion
    generateFieldTemplate(printer, message);

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);
        printer("#undef PROTOGEN_FN_$1$\n", storage);
    }
}


static std::string makeGuard( const std::string &fileName )
{
    std::string out = "GUARD_";
    for (auto it = fileName.begin(); it != fileName.end(); ++it)
    {
        if ( (*it >= 'A' && *it <= 'Z') ||
             (*it >= 'a' && *it <= 'z') ||
             (*it >= '0' && *it <= '9') ||
             *it == '_')
            out += *it;
        else
            out += '_';
    }
    return out;
}


static void generateModel( Printer &printer, const Proto3 &proto )
{
    char version[8] = { 0 };
    snprintf(version, sizeof(version) - 1, "%02X%02X%02X",
        (int) PROTOGEN_MAJOR, (int) PROTOGEN_MINOR, (int) PROTOGEN_PATCH);

    std::string guard = makeGuard(proto.fileName);
    printer(
        "// Generated by the protogen $1$ compiler.  DO NOT EDIT!\n"
        "// Source: $3$\n"
        "\n#ifndef $4$\n"
        "#define $4$\n\n"
        "#include <string>\n"
        "#include <cstring>\n"
        "#include <stdint.h>\n"
        "#include <iterator>\n"
        "#include <sstream>\n\n"
        "#pragma GCC diagnostic ignored \"-Wunused-function\"\n\n"
        "#ifndef PROTOGEN_VERSION\n"
        "    #define PROTOGEN_VERSION 0x$5$ // $1$\n"
        "#endif\n"
        "#if (PROTOGEN_VERSION != 0x$5$) // $1$\n"
        "    #error Mixed versions of protogen!\n"
        "#endif\n"
        // base template
        "\n$2$\n", PROTOGEN_VERSION, BASE_TEMPLATE, proto.fileName, guard, version);

    // macros for custom templates
    generateTraitMacro(printer);
    generateWriterMacro(printer);
    generateWriterPrototypeMacro(printer);
    generateFieldTemplateMacro(printer);

    // forward declarations
    printer("\n// forward declarations\n");
    for (auto mi = proto.messages.begin(); mi != proto.messages.end(); ++mi)
    {
        generateNamespace(printer, *mi, true);
        printer("\tclass $1$;\n\b", mi->name);
        generateNamespace(printer, *mi, false);
    }
    // message declarations
    for (auto mi = proto.messages.begin(); mi != proto.messages.end(); ++mi)
        generateMessage(printer, *mi);

    printer("#endif // $1$\n", guard);
}


void CppGenerator::generate( Proto3 &proto, std::ostream &out )
{
    Printer printer(out);
    generateModel(printer, proto);
}


}