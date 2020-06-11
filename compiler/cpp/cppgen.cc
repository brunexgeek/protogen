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

#include <algorithm>
#include <auto-code.hh>
#include <protogen/protogen.hh>
#include "../printer.hh"
#include <sstream>

namespace protogen {


#define IS_VALID_TYPE(x)      ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )
#define IS_NUMERIC_TYPE(x)    ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_SFIXED64 )

#ifdef _WIN32
#define SNPRINTF snprintf_
#else
#define SNPRINTF snprintf
#endif

struct GeneratorContext
{
    Printer &printer;
    Proto3 &root;
    bool number_names;
    bool obfuscate_strings;
    bool cpp_enable_errors;
    bool cpp_use_lists;
    std::string custom_parent;
    std::string version;
    std::string versionNo;

    GeneratorContext( Printer &printer, Proto3 &root ) : printer(printer), root(root),
        number_names(false), obfuscate_strings(false),
        cpp_enable_errors(false), cpp_use_lists(false)
    {
    }
};

static struct {
    protogen::FieldType type;
    const char *typeName;
    const char *nativeType;
    const char *defaultValue;
} TYPE_MAPPING[] =
{
    { protogen::TYPE_DOUBLE,   "double",    "double"      , "0.0" },
    { protogen::TYPE_FLOAT,    "float",     "float"       , "0.0F" },
    { protogen::TYPE_INT32,    "int32",     "int32_t"     , "0" },
    { protogen::TYPE_INT64,    "int64",     "int64_t"     , "0" },
    { protogen::TYPE_UINT32,   "uint32",    "uint32_t"    , "0" },
    { protogen::TYPE_UINT64,   "uint64",    "uint64_t"    , "0" },
    { protogen::TYPE_SINT32,   "sint32",    "int32_t"     , "0" },
    { protogen::TYPE_SINT64,   "sint64",    "int64_t"     , "0" },
    { protogen::TYPE_FIXED32,  "fixed32",   "uint32_t"    , "0" },
    { protogen::TYPE_FIXED64,  "fixed64",   "uint64_t"    , "0" },
    { protogen::TYPE_SFIXED32, "sfixed32",  "int32_t"     , "0" },
    { protogen::TYPE_SFIXED64, "sfixed64",  "int64_t"     , "0" },
    { protogen::TYPE_BOOL,     "bool",      "bool"        , "false" },
    { protogen::TYPE_STRING,   "string",    "std::string" , "\"\"" },
    { protogen::TYPE_BYTES,    "bytes",     "uint8_t"     , nullptr },
    { protogen::TYPE_MESSAGE,  nullptr,     nullptr       , nullptr }
};

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
  * Translates protobuf3 types to C++ types.
  */
static std::string fieldNativeType( const Field &field, bool useLists )
{
    std::string valueType;

    // value type
    if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_BYTES)
    {
        int index = (int)field.type.id - (int)protogen::TYPE_DOUBLE;
        valueType = TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type.id == protogen::TYPE_MESSAGE)
    {
        valueType = nativeType(field);
    }
    else
        throw protogen::exception("Invalid field type");

    if (field.type.repeated || field.type.id == protogen::TYPE_BYTES)
    {
        std::string output = "std::";
        if (field.type.id == protogen::TYPE_BYTES || !useLists)
            output += "vector<";
        else
            output += "list<";
        output += valueType;
        output += '>';
        return output;
    }
    if (field.type.id == protogen::TYPE_MESSAGE || field.type.id == protogen::TYPE_STRING)
        return valueType;
    else
    {
        std::string output = "PROTOGEN_NS::field<";
        output += valueType;
        output += '>';
        return output;
    }
}

static void generateMoveCtor( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("$1$($1$ &&that) {\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        if (fi->type.id == TYPE_MESSAGE || fi->type.repeated || fi->type.id == protogen::TYPE_BYTES || fi->type.id == TYPE_STRING)
            ctx.printer("this->$1$.swap(that.$1$);\n", fieldStorage(*fi));
        else
            ctx.printer("this->$1$.swap(that.$1$);\n", fieldStorage(*fi));
    }
    ctx.printer("}\n\b");
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

static void generateModel( GeneratorContext &ctx, const Message &message )
{
    // begin namespace
    generateNamespace(ctx, message, true);

    ctx.printer("struct $1$_type\n{\n", message.name);
    for (auto field : message.fields)
    {
        ctx.printer("\t$1$ $2$;\n", fieldNativeType(field, true), fieldStorage(field) );
    }
    ctx.printer("};\n");

    // end namespace
    generateNamespace(ctx, message, false);
}

static void generateModelWrapper( GeneratorContext &ctx, const Message &message )
{
    std::string typeName = nativePackage(message.package) + "::" + message.name + "_type";
    std::stringstream temp1;
    std::stringstream temp2;
    std::stringstream temp3;
    std::stringstream temp4;
    std::stringstream temp5;
    std::stringstream temp6;
    for (auto field : message.fields)
    {
        std::string name = field.name;

        Printer::format(temp1, CODE_DESERIALIZE_IF, name);
        Printer::format(temp2, CODE_SERIALIZE_IF, name);
        Printer::format(temp3, CODE_EMPTY_IF, name);
        Printer::format(temp4, CODE_CLEAR_CALL, name);
        Printer::format(temp5, CODE_EQUAL_IF, name);
        Printer::format(temp6, CODE_SWAP_CALL, name);
    }

    ctx.printer(CODE_JSON_MODEL, typeName, temp1.str(),
        temp2.str(), temp3.str(), temp4.str(), temp5.str(), temp6.str());
}

static void generateEntity( GeneratorContext &ctx, const Message &message )
{
    std::string originalType = nativePackage(message.package) + "::" + message.name + "_type";
    generateNamespace(ctx, message, true);
    ctx.printer(CODE_ENTITY, message.name , originalType);
    generateNamespace(ctx, message, false);
}

static void generateEntityWrapper( GeneratorContext &ctx, const Message &message )
{
    std::string typeName = nativePackage(message.package) + "::" + message.name;
    ctx.printer(CODE_ENTITY_JSON, typeName, typeName + "_type");
}

static void generateMessage( GeneratorContext &ctx, const Message &message, bool obfuscate_strings = false )
{
    ctx.printer("\n//\n// $1$\n//\n", message.name);

    // create the model structure
    generateModel(ctx, message);
    // create JSON wrapper for model structure
    generateModelWrapper(ctx, message);
    // create entoty structure and JSON wrapper
    //ctx.printer("PG_JSON_ENTITY($1$, $2$)", message.name, message.name + "_type");
    generateEntity(ctx, message);
    generateEntityWrapper(ctx, message);

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

typedef std::vector<protogen::Message *> MessageList;

static bool contains( const MessageList &items, const protogen::Message *message )
{
    for (auto mi = items.begin(); mi != items.end(); ++mi)
        if (*mi == message) return true;
    return false;
}


static void sort( MessageList &items, MessageList &pending, protogen::Message *message )
{
    if (contains(pending, message)) return; // circular reference
    if (contains(items, message)) return; // already processed

    pending.push_back(message);

    for (auto fi = message->fields.begin(); fi != message->fields.end(); ++fi)
    {
        if (fi->type.ref == nullptr) continue;
        if (!contains(items, fi->type.ref)) sort(items, pending, fi->type.ref);
    }

    items.push_back(message);

    auto it = std::find(pending.begin(), pending.end(), message);
    if (it != pending.end()) pending.erase(it);
}


static void sort( GeneratorContext &ctx )
{
    std::vector<protogen::Message *> items;
    std::vector<protogen::Message *> pending;
    for (auto mi = ctx.root.messages.begin(); mi != ctx.root.messages.end(); ++mi)
    {
        sort(items, pending, *mi);
    }

    ctx.root.messages.swap(items);
}


static void generateModel( GeneratorContext &ctx )
{
    char version[12] = { 0 };
    SNPRINTF(version, sizeof(version) - 1, "%02X%02X%02X",
        (int) PROTOGEN_MAJOR, (int) PROTOGEN_MINOR, (int) PROTOGEN_PATCH);
    ctx.versionNo = version;
    SNPRINTF(version, sizeof(version) - 1, "%d_%d_%d",
        (int) PROTOGEN_MAJOR, (int) PROTOGEN_MINOR, (int) PROTOGEN_PATCH);
    ctx.version = version;

    std::string guard = makeGuard(ctx.root.fileName);
    ctx.printer(CODE_HEADER, PROTOGEN_VERSION, ctx.root.fileName, guard, ctx.version);

    sort(ctx);

    // message declarations
    for (auto mi = ctx.root.messages.begin(); mi != ctx.root.messages.end(); ++mi)
        generateMessage(ctx, **mi, ctx.obfuscate_strings);

    ctx.printer(
        "#undef PROTOGEN_NS\n"
        "#endif // $1$\n", guard);
}


void CppGenerator::generate( Proto3 &root, std::ostream &out )
{
    Printer printer(out);
    GeneratorContext ctx(printer, root);

    if (ctx.root.options.count(PROTOGEN_O_OBFUSCATE_STRINGS))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_OBFUSCATE_STRINGS);
        if (opt.type != OptionType::BOOLEAN)
            throw exception("The value for 'obfuscate_strings' must be a boolean", opt.line, 1);
        ctx.obfuscate_strings = opt.value == "true";
    }

    if (ctx.root.options.count(PROTOGEN_O_NUMBER_NAMES))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_NUMBER_NAMES);
        if (opt.type != OptionType::BOOLEAN)
            throw exception("The value for 'number_names' must be a boolean", opt.line, 1);
        ctx.number_names = opt.value == "true";
    }

    if (ctx.root.options.count(PROTOGEN_O_CPP_ENABLE_ERRORS))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_CPP_ENABLE_ERRORS);
        if (opt.type != OptionType::BOOLEAN)
            throw exception("The value for 'cpp_enable_errors' must be a boolean", opt.line, 1);
        ctx.cpp_enable_errors = opt.value == "true";
    }

    if (ctx.root.options.count(PROTOGEN_O_CUSTOM_PARENT))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_CUSTOM_PARENT);
        if (opt.type != OptionType::STRING && opt.type != OptionType::IDENTIFIER)
            throw exception("The value for 'custom_parent' must be a string or an identifier", opt.line, 1);
        ctx.custom_parent = opt.value;
    }

    if (ctx.root.options.count(PROTOGEN_O_CPP_USE_LISTS))
    {
        OptionEntry opt = ctx.root.options.at(PROTOGEN_O_CPP_USE_LISTS);
        if (opt.type != OptionType::BOOLEAN)
            throw exception("The value for 'cpp_use_lists' must be a boolean", opt.line, 1);
        ctx.cpp_use_lists = opt.value == "true";
    }

    generateModel(ctx);
}


}