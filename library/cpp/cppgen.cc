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


extern const char *BASE_1_TEMPLATE;
extern const char *BASE_2_TEMPLATE;


namespace protogen {


#define IS_VALID_TYPE(x)      ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )
#define IS_NUMERIC_TYPE(x)    ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_SFIXED64 )


struct GeneratorContext
{
    Printer &printer;
    Proto3 &root;
    bool number_names;
    bool obfuscate_strings;
    bool cpp_enable_parent;
    bool cpp_enable_errors;
    std::string version;
    std::string versionNo;

    GeneratorContext( Printer &printer, Proto3 &root ) : printer(printer), root(root),
        number_names(false), obfuscate_strings(false), cpp_enable_parent(false),
        cpp_enable_errors(false)
    {
    }
};


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


//static std::string nativePackage( const std::vector<std::string> &package )
static std::string nativePackage( const std::string &package )
{
    // extra space because the compiler may complain about '<::' (i.e. using in templates)
    if (package.empty()) return " ";
    std::string name = " ::";
    for (auto it = package.begin(); it != package.end(); ++it)
    {
        if (*it == '.')
            name += "::";
        else
            name += *it;
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
static std::string proto3Type( const Field &field )
{
    std::string name;

    if (field.type.repeated) name += "repeated ";

    if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_STRING)
    {
        int index = (int)field.type.id - (int)protogen::TYPE_DOUBLE;
        name += TYPE_MAPPING[index].typeName;
    }
    else
    if (field.type.id == protogen::TYPE_BYTES)
        name += "bytes";
    else
    if (field.type.id == protogen::TYPE_MESSAGE)
        name += field.type.ref->name;
    else
        throw protogen::exception("Invalid field type");

    return name;
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
        return nativePackage(field.type.ref->package) + "::" + field.type.ref->name;
    else
        throw protogen::exception("Invalid field type");
}

/**
 * Translates protobuf3 types to protogen types.
 */
static std::string fieldNativeType( const Field &field )
{
    std::string output = "PROTOGEN_NS::";

    if (field.type.repeated || field.type.id == protogen::TYPE_BYTES)
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
    {
        output += nativeType(field);
    }
    else
        throw protogen::exception("Invalid field type");

    output += '>';
    return output;
}


static void generateVariable( GeneratorContext &ctx, const Field &field )
{
    std::string storage = fieldStorage(field);

    char number[10];
    snprintf(number, sizeof(number), "%d", field.index);
    number[9] = 0;

    ctx.printer(
        "static const int $3$_NO = $4$;\n"
        "$1$ $2$;\n", fieldNativeType(field), storage, toUpper(storage), number);
}


static void generateCopyCtor( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("$1$(const $1$ &that) { *this = that; }\n", message.name);
}


static void generateMoveCtor( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "#if __cplusplus >= 201103L\n"
        "$1$($1$ &&that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        if (fi->type.id == TYPE_MESSAGE || fi->type.repeated)
            ctx.printer("this->$1$.swap(that.$1$);\n", fieldStorage(*fi));
        else
            ctx.printer("this->$1$ = that.$1$;\n", fieldStorage(*fi));
    }
    ctx.printer("\b}\n#endif\n");
}


static void generateAssignOperator( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("$1$ &operator=(const $1$ &that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
        ctx.printer("this->$1$ = that.$1$;\n", fieldStorage(*fi));
    ctx.printer("return *this;\n\b}\n");
}


static void generateEqualityOperator( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "bool operator==(const $1$ &that) const {\n"
        "\treturn\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        ctx.printer("this->$1$ == that.$1$", fieldStorage(*fi));
        if (fi + 1 != message.fields.end())
            ctx.printer(" &&\n");
        else
            ctx.printer(";\n");
    }
    ctx.printer("\b\b}\n");
}


static void generateDeserializer( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        // deserializer receiving a 'istream'
        "bool deserialize( std::istream &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) {\n"
        "\tbool skip = in.flags() & std::ios_base::skipws;\n"
        "std::noskipws(in);\n"
        "std::istream_iterator<char> itb(in);\n"
        "std::istream_iterator<char> ite;\n"
        "PROTOGEN_NS::InputStream< std::istream_iterator<char> > is(itb, ite);\n"
        "bool result = this->deserialize(is, required, err);\n"
        "if (skip) std::skipws(in);\n"
        "return result;\n"
        "\b}\n"

        // deserializer receiving a 'string'
        "bool deserialize( const std::string &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) {\n"
        "\tPROTOGEN_NS::InputStream<std::string::const_iterator> is(in.begin(), in.end());\n"
        "return this->deserialize(is, required, err);\n"
        "\b}\n"

        // deserializer receiving a 'vector'
        "bool deserialize( const std::vector<char> &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) {\n"
        "\tPROTOGEN_NS::InputStream<std::vector<char>::const_iterator> is(in.begin(), in.end());\n"
        "return this->deserialize(is, required, err);\n"
        "\b}\n"

        // deserializer receiving iterators
       	"template <typename IT>\n"
        "bool deserialize( IT begin, IT end, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ){\n"
        "\tPROTOGEN_NS::InputStream<IT> is(begin, end);\n"
        "return this->deserialize(is, required, err);\n\b}\n"

        // 'real' deserializer
        "template<typename T>\n"
        "bool deserialize( PROTOGEN_NS::InputStream<T> &in, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) {\n"
        "\t(void) err;\n"
        "in.skipws();\n"
        "if (in.get() != '{') PROTOGEN_REG(err, in, \"Invalid object\");\n"
        "in.skipws();\n"
        "std::string name;\n");

    ctx.printer(
        "bool hfld[$1$] = {false};\n"
        "while (true) {\n\t"
        "if (in.get() == '}') break;\n"
        "in.unget();\n"
        "name.clear();\n"
        "if (!PROTOGEN_NS::json::readName(in, name)) PROTOGEN_REG(err, in, \"Invalid field name\");\n", message.fields.size());

    bool first = true;
    size_t count = 0;

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi, ++count)
    {
        if (!IS_VALID_TYPE(fi->type.id)) continue;

        std::string storage = fieldStorage(*fi);
        std::string type = nativeType(*fi);

        if (!first) ctx.printer("else\n");
        ctx.printer("// $1$\n", fi->name);
        ctx.printer("if (name == PROTOGEN_FN_$1$) {\n\t", storage); // open the main 'if'

        if (fi->type.repeated || fi->type.id == protogen::TYPE_BYTES)
        {
            ctx.printer(
                "if (!PROTOGEN_NS::traits< std::vector<$1$> >::read(in, this->$2$())) "
                "PROTOGEN_REV(err, in, name, \"$3$\");\n",
                type, storage, proto3Type(*fi));
        }
        else
        {
            if (fi->type.id >= protogen::TYPE_DOUBLE && fi->type.id <= protogen::TYPE_STRING)
            {
                ctx.printer(
                    "$1$ value;\n"
                    "if (!PROTOGEN_NS::traits<$1$>::read(in, value)) "
                    "PROTOGEN_REV(err, in, name, \"$3$\");\n"
                    "this->$2$(value);\n", type, storage, proto3Type(*fi));
            }
            else
            if (fi->type.id == protogen::TYPE_MESSAGE)
                ctx.printer(
                    "if (!this->$1$().deserialize(in, required, err)) "
                    "PROTOGEN_REV(err, in, name, \"$2$\");\n",
                    storage, proto3Type(*fi));
        }
        ctx.printer(
            "if (!PROTOGEN_NS::json::next(in)) PROTOGEN_REI(err, in, name);\n"
            "hfld[$1$] = true;\n"
            "\b}\n", // closes the main 'if'
            count);
        first = false;
    }
    ctx.printer(
        "else\n"
        "// ignore the current field\n"
        "{\n\tif (!PROTOGEN_NS::json::ignore(in)) PROTOGEN_REI(err, in, name);\n"
        "if (!PROTOGEN_NS::json::next(in)) PROTOGEN_REI(err, in, name);\n\b}\n\b}\n");

    // check whether we missed any required field
    ctx.printer("if (required) {\n\t");
    for (size_t i = 0, t = message.fields.size(); i < t; ++i)
    {
        ctx.printer("if (!hfld[$1$]) PROTOGEN_REM(err, in, \"$2$\");\n", i, message.fields[i].name);
    }
    ctx.printer("\b}\nreturn true;\n\b}\n");
}


static void generateSerializer( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
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

        ctx.printer(
            "// $1$\n"
            "if (!this->$1$.undefined()) "
            "PROTOGEN_NS::json::write(out, first, PROTOGEN_FN_$1$, this->$1$());\n", storage);
    }
    ctx.printer("out << '}';\n\b}\n");
}


static void generateTrait( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "PROTOGEN_TRAIT_MACRO($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateClear( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("void clear() {\n\t");
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        ctx.printer("this->$1$.clear();\n", fieldStorage(*fi));
    }
    ctx.printer("\b}\n");
}


static void generateSwap( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("void swap($1$ &that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        ctx.printer("this->$1$.swap(that.$1$);\n", fieldStorage(*fi));
    }
    ctx.printer("\b}\n");
}


static void generateNamespace( GeneratorContext &ctx, const Message &message, bool start )
{
    if (message.package.empty()) return;

    std::string name;
    std::string package = message.package + '.';
    for (auto it = package.begin(); it != package.end(); ++it)
    {
        if (*it == '.')
        {
            if (start)
                ctx.printer("namespace $1$ {\n", name);
            else
                ctx.printer("} // namespace $1$\n", name);
            name.clear();
        }
        else
            name += *it;
    }
}


static void generateFieldTemplate( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "PROTOGEN_FIELD_TEMPLATE($1$::$2$)\n", nativePackage(message.package), message.name);
}


static void generateUndefined( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "bool undefined() const {\n"
        "\treturn\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        ctx.printer("this->$1$.undefined()", fieldStorage(*fi));
        if (fi + 1 != message.fields.end())
            ctx.printer(" &&\n");
        else
            ctx.printer(";\n");
    }
    ctx.printer("\b\b}\n");
}


static void generateMessage( GeneratorContext &ctx, const Message &message, bool obfuscate_strings = false )
{
    ctx.printer("\n//\n// $1$\n//\n", message.name);

    // begin namespace
    generateNamespace(ctx, message, true);

    ctx.printer("\nclass $1$", message.name);
    if (ctx.cpp_enable_parent) ctx.printer(": public PROTOGEN_NS::Message");
    ctx.printer(
        " {\npublic:\n"
        "\ttypedef PROTOGEN_NS::ErrorInfo ErrorInfo;\n"
        "static const uint32_t PROTOGEN_VERSION = 0x$1$;\n", ctx.versionNo);

    // create the macro containing the field name
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);
        std::string name;
        if (ctx.number_names)
        {
            char temp[12] = {0};
            snprintf(temp, sizeof(temp) - 1, "%d", fi->index);
            name = temp;
        }
        else
            name = fieldStorage(*fi);

        if (obfuscate_strings)
        {
            ctx.printer(
                "#undef PROTOGEN_FN_$1$\n"
                "#define PROTOGEN_FN_$1$ PROTOGEN_NS::json::reveal(\"$2$\", $3$)\n",
                storage, obfuscate(name), name.length());
        }
        else
        {
            ctx.printer(
                "#undef PROTOGEN_FN_$1$\n"
                "#define PROTOGEN_FN_$1$ \"$2$\"\n", storage, name);
        }
    }

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        // storage variable
        generateVariable(ctx, *fi);
    }

    // default constructor and destructor
    ctx.printer("$2$() {}\n$1$~$2$() {}\n", ((ctx.cpp_enable_parent) ? "virtual " : ""), message.name);
    // copy constructor
    generateCopyCtor(ctx, message);
    // move constructor
    generateMoveCtor(ctx, message);
    // assign operator
    generateAssignOperator(ctx, message);
    // swap function
    generateSwap(ctx, message);
    // equality operator
    generateEqualityOperator(ctx, message);
    // undefined function
    generateUndefined(ctx, message);
    // message serializer
    generateSerializer(ctx, message);
    // message deserializer
    generateDeserializer(ctx, message);
    // clear function
    generateClear(ctx, message);
    // close class declaration
    ctx.printer("\b};\n");
    // end namespace
    generateNamespace(ctx, message, false);

    // message trait
    generateTrait(ctx, message);
    // 'Field' template specializarion
    generateFieldTemplate(ctx, message);

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);
        ctx.printer("#undef PROTOGEN_FN_$1$\n", storage);
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


static void generateLicense( GeneratorContext &ctx )
{
    ctx.printer(
        "/*\n"
        " * This is free and unencumbered software released into the public domain.\n"
        " * \n"
        " * Anyone is free to copy, modify, publish, use, compile, sell, or\n"
        " * distribute this software, either in source code form or as a compiled\n"
        " * binary, for any purpose, commercial or non-commercial, and by any\n"
        " * means.\n"
        " * \n"
        " * In jurisdictions that recognize copyright laws, the author or authors\n"
        " * of this software dedicate any and all copyright interest in the\n"
        " * software to the public domain. We make this dedication for the benefit\n"
        " * of the public at large and to the detriment of our heirs and\n"
        " * successors. We intend this dedication to be an overt act of\n"
        " * relinquishment in perpetuity of all present and future rights to this\n"
        " * software under copyright law.\n"
        " * \n"
        " * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
        " * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
        " * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
        " * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\n"
        " * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\n"
        " * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
        " * OTHER DEALINGS IN THE SOFTWARE.\n"
        " * \n"
        " * For more information, please refer to <http://unlicense.org>\n */\n\n");
}


static void generateModel( GeneratorContext &ctx )
{
    char version[12] = { 0 };
    snprintf(version, sizeof(version) - 1, "%02X%02X%02X",
        (int) PROTOGEN_MAJOR, (int) PROTOGEN_MINOR, (int) PROTOGEN_PATCH);
    ctx.versionNo = version;
    snprintf(version, sizeof(version) - 1, "%d_%d_%d",
        (int) PROTOGEN_MAJOR, (int) PROTOGEN_MINOR, (int) PROTOGEN_PATCH);
    ctx.version = version;

    generateLicense(ctx);

    std::string guard = makeGuard(ctx.root.fileName);
    ctx.printer(
        "// AUTO-GENERATED FILE. DO NOT EDIT!\n"
        "// Generated by the protogen $1$ compiler <https://github.com/brunexgeek/protogen>\n"
        "// Source: $2$\n"
        "\n#ifndef $3$\n"
        "#define $3$\n\n"
        "#if !defined(_WIN32) && !defined(_WIN64)\n"
        "   #pragma GCC diagnostic ignored \"-Wunused-function\"\n"
        "#endif\n"
        "#ifdef PROTOGEN_VERSION\n"
        "   #undef PROTOGEN_VERSION\n"
        "#endif\n\n", PROTOGEN_VERSION, ctx.root.fileName, guard);

    if (ctx.obfuscate_strings)
        ctx.printer("#define PROTOGEN_OBFUSCATE_STRINGS // strings obfuscation\n");
    if (ctx.cpp_enable_parent)
        ctx.printer("#define PROTOGEN_CPP_ENABLE_PARENT // common parent class for messages\n");
    if (ctx.cpp_enable_errors)
        ctx.printer("#define PROTOGEN_CPP_ENABLE_ERRORS // enable parsing error information\n");

    ctx.printer(
        "\n#include <string>\n"
        "#include <cstring>\n"
        "#include <stdint.h>\n"
        "#include <iterator>\n"
        "#include <sstream>\n"
        "#include <iostream>\n"
        "#include <vector>\n"
        "#include <cstdlib>\n"
        "#include <locale.h>\n"
        "#include <stdexcept>\n\n");

    // base template
    ctx.printer(
        "#undef PROTOGEN_NS\n"
        "#define PROTOGEN_NS protogen_$1$\n\n", ctx.version);
    ctx.printer.output() << BASE_1_TEMPLATE;
    ctx.printer(
        "\n#ifndef PROTOGEN_BASE_$1$\n"
        "#define PROTOGEN_BASE_$1$\n", ctx.version);
    ctx.printer.output() << BASE_2_TEMPLATE;
    ctx.printer("#endif // PROTOGEN_BASE_$1$\n", version);

    // forward declarations
    ctx.printer("\n// forward declarations\n");
    for (auto mi = ctx.root.messages.begin(); mi != ctx.root.messages.end(); ++mi)
    {
        generateNamespace(ctx, **mi, true);
        ctx.printer("\tclass $1$;\n\b", (*mi)->name);
        generateNamespace(ctx, **mi, false);
    }
    // message declarations
    for (auto mi = ctx.root.messages.begin(); mi != ctx.root.messages.end(); ++mi)
        generateMessage(ctx, **mi, ctx.obfuscate_strings);

    ctx.printer(
        "#undef PROTOGEN_OBFUSCATE_STRINGS\n"
        "#undef PROTOGEN_CPP_ENABLE_PARENT\n"
        "#undef PROTOGEN_CPP_ENABLE_ERRORS\n"
        "#undef PROTOGEN_NS\n"
        "#undef PROTOGEN_REV\n"
        "#undef PROTOGEN_REI\n"
        "#undef PROTOGEN_REF\n"
        "#undef PROTOGEN_REM\n"
        "#undef PROTOGEN_REG\n"
        "#endif // $1$\n", guard);
}


void CppGenerator::generate( Proto3 &root, std::ostream &out )
{
    Printer printer(out);
    GeneratorContext ctx(printer, root);

    if (ctx.root.options.count(PROTOGEN_O_OBFUSCATE_STRINGS))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_OBFUSCATE_STRINGS);
        ctx.obfuscate_strings = (opt.type == OptionType::BOOLEAN && opt.value == "true");
    }

    if (ctx.root.options.count(PROTOGEN_O_NUMBER_NAMES))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_NUMBER_NAMES);
        ctx.number_names = (opt.type == OptionType::BOOLEAN && opt.value == "true");
    }

    if (ctx.root.options.count(PROTOGEN_O_CPP_ENABLE_PARENT))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_CPP_ENABLE_PARENT);
        ctx.cpp_enable_parent = (opt.type == OptionType::BOOLEAN && opt.value == "true");
    }

    if (ctx.root.options.count(PROTOGEN_O_CPP_ENABLE_ERRORS))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_CPP_ENABLE_ERRORS);
        ctx.cpp_enable_errors = (opt.type == OptionType::BOOLEAN && opt.value == "true");
    }

    generateModel(ctx);
}


}