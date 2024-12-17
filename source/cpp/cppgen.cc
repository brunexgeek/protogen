/*
 * Copyright 2020-2024 Bruno Ribeiro <https://github.com/brunexgeek>
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
#include <unordered_set>
#include <vector>
#include <cmake.hh>
#include <auto-code.hh>
#include <auto-protogen.hh>
#include <auto-json.hh>
#include <auto-json-array.hh>
#include <auto-json-base64.hh>
#include <auto-json-number.hh>
#include <auto-json-string.hh>
#include <protogen/protogen.hh>
#include "../printer.hh"
#include <sstream>
#include <cstdio>

namespace protogen {

#ifdef _WIN32
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

struct GeneratorContext
{
    Printer &printer;
    Proto3 &root;
    bool number_names = false;
    bool obfuscate_strings = false;
    bool cpp_use_lists = false;

    GeneratorContext( Printer &printer, Proto3 &root ) : printer(printer), root(root) {}
};

static constexpr const struct {
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
    { protogen::TYPE_STRING,   "string",    nullptr       , "\"\"" },
    { protogen::TYPE_BYTES,    "bytes",     "uint8_t"     , nullptr },
    { protogen::TYPE_MESSAGE,  nullptr,     nullptr       , nullptr }
};

static std::string nativePackage( const std::string &package )
{
    // extra space because the compiler may complain about '<::' (i.e. using in templates)
    if (package.empty())
        return " ";
    std::string name;
    name.reserve(package.length());
    name += " ::";
    for (auto c : package)
    {
        if (c == '.')
            name += "::";
        else
            name += c;
    }
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
    {
        if (field.type.ref == nullptr)
            throw protogen::exception("Message type reference is null");
        return nativePackage(field.type.ref->package) + "::" + field.type.ref->name;
    }
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
    if (field.type.id == protogen::TYPE_STRING)
    {
        valueType = "protogen";
        valueType += PROTOGEN_VERSION_NAMING;
        valueType += "::string_field";
    }
    else
    if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_BYTES)
    {
        int index = (int)field.type.id - (int)protogen::TYPE_DOUBLE;
        valueType = TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type.id == protogen::TYPE_MESSAGE)
        valueType = nativeType(field);
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
        std::string output = "protogen";
        output += PROTOGEN_VERSION_NAMING;
        output += "::field<";
        output += valueType;
        output += '>';
        return output;
    }
}

static void generateNamespace( GeneratorContext &ctx, const Message &message, bool opening )
{
    if (message.package.empty()) return;

    std::string name;
    std::string package = message.package + '.';
    for (auto c : package)
    {
        if (c == '.')
        {
            if (opening)
                ctx.printer("namespace $1$ {\n", name);
            else
                ctx.printer("} // namespace $1$\n", name);
            name.clear();
        }
        else
            name += c;
    }
}

static void generateModel( GeneratorContext &ctx, const Message &message )
{
    // begin namespace
    generateNamespace(ctx, message, true);

    ctx.printer("\tstruct $1$_type\n\t{\n", message.name);
    for (auto field : message.fields)
    {
        auto type = fieldNativeType(field, ctx.cpp_use_lists);
        ctx.printer("\t\t$1$ $2$;\n", type, field.name);
    }
    ctx.printer("\t};\n");

    // end namespace
    generateNamespace(ctx, message, false);
}

template <typename T>
T rol( T value, int count )
{
	static_assert(std::is_unsigned<T>::value, "Unsupported signed type");
	return (T) ((value << count) | (value >> (-count & ((int)sizeof(T) * 8 - 1))));
}

static inline std::string obfuscate( const std::string &value )
{
    uint8_t mask = rol<uint8_t>(0x93U, value.length() % 8);
	std::stringstream result;
	for (size_t i = 0, t = value.length(); i < t; ++i)
    {
		result << "\\x";
        result << std::hex << ((uint8_t) value[i] ^ mask);
    }
	return result.str();
}

static std::string get_option( const protogen::OptionMap &options, const std::string &name, const std::string &value )
{
    if (options.count(name) == 0)
        return value;

    OptionEntry opt = options.at(name);
    if (opt.type != OptionType::STRING)
        throw exception("The value for '" + name + "' must be a string", opt.line, 1);
    return opt.value;
}

static bool get_option( const protogen::OptionMap &options, const std::string &name, bool value )
{
    if (options.count(name) == 0)
        return value;

    OptionEntry opt = options.at(name);
    if (opt.type != OptionType::BOOLEAN)
        throw exception("The value for '" + name + "' must be a boolean", opt.line, 1);
    return opt.value == "true";
}

static bool is_transient( const Field &field )
{
    return get_option(field.options, PROTOGEN_O_TRANSIENT, false);
}

static std::string get_json_name( const Field &field )
{
    auto name = get_option(field.options, PROTOGEN_O_NAME, field.name);
    for (auto c : name)
        if (c == '\'' || c == '"')
            throw exception("option '" + std::string(PROTOGEN_O_NAME) + "' in the field '" + field.name + "' must not contain quotes", 1, 1);
    return name;
}

static void generate_function__read_field( GeneratorContext &ctx, const Message &message, const std::string &typeName,
    bool is_persistent )
{
    if (message.fields.size() == 0 || !is_persistent)
    {
        ctx.printer(CODE_JSON__READ_FIELD__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__READ_FIELD__HEADER, typeName);

    int i = 0;
    for (auto field : message.fields)
    {
        if (is_transient(field))
            continue;
        ctx.printer(CODE_JSON__READ_FIELD__ITEM, i, field.name);
        ++i;
    }

    ctx.printer(CODE_JSON__READ_FIELD__FOOTER);
}

static void generate_function__write( GeneratorContext &ctx, const Message &message, const std::string &typeName,
    bool is_persistent )
{
    if (message.fields.size() == 0 || !is_persistent)
    {
        ctx.printer(CODE_JSON__WRITE__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__WRITE__HEADER, typeName, PROTOGEN_VERSION_NAMING);

    int i = 0;
    for (auto field : message.fields)
    {
        if (is_transient(field))
            continue;
        auto name = get_json_name(field);

        std::string label = ctx.number_names ? std::to_string(field.index) : name;
        if (ctx.obfuscate_strings)
            label = Printer::format("reveal(\"$1$\")", obfuscate(label));
        else
            label = Printer::format("\"$1$\"", label);

        ctx.printer(CODE_JSON__WRITE__ITEM, field.name, label);
        ++i;
    }

    ctx.printer(CODE_JSON__WRITE__FOOTER);
}

static void generate_function__empty( GeneratorContext &ctx, const Message &message, const std::string &typeName )
{
    if (message.fields.size() == 0)
    {
        ctx.printer(CODE_JSON__EMPTY__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__EMPTY__HEADER, typeName);

    int i = 0;
    for (auto field : message.fields)
    {
        ctx.printer(CODE_JSON__EMPTY__ITEM, field.name);
        ++i;
    }

    ctx.printer(CODE_JSON__EMPTY__FOOTER);
}

static void generate_function__clear( GeneratorContext &ctx, const Message &message, const std::string &typeName )
{
    if (message.fields.size() == 0)
    {
        ctx.printer(CODE_JSON__CLEAR__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__CLEAR__HEADER, typeName);

    int i = 0;
    for (auto field : message.fields)
    {
        ctx.printer(CODE_JSON__CLEAR__ITEM, field.name);
        ++i;
    }

    ctx.printer(CODE_JSON__CLEAR__FOOTER);
}

static void generate_function__equal( GeneratorContext &ctx, const Message &message, const std::string &typeName )
{
    if (message.fields.size() == 0)
    {
        ctx.printer(CODE_JSON__EQUAL__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__EQUAL__HEADER, typeName);

    int i = 0;
    for (auto field : message.fields)
    {
        ctx.printer(CODE_JSON__EQUAL__ITEM, field.name);
        ++i;
    }

    ctx.printer(CODE_JSON__EQUAL__FOOTER);
}

static void generate_function__swap( GeneratorContext &ctx, const Message &message, const std::string &typeName )
{
    if (message.fields.size() == 0)
    {
        ctx.printer(CODE_JSON__SWAP__EMPTY, typeName);
        return;
    }

    ctx.printer(CODE_JSON__SWAP__HEADER, typeName);

    int i = 0;
    for (auto field : message.fields)
    {
        ctx.printer(CODE_JSON__SWAP__ITEM, field.name);
        ++i;
    }

    ctx.printer(CODE_JSON__SWAP__FOOTER);
}

static void generate_function__index( GeneratorContext &ctx, const Message &message, bool is_persistent )
{
    if (message.fields.size() == 0 || !is_persistent)
    {
        ctx.printer(CODE_JSON__INDEX__EMPTY);
        return;
    }

    if (ctx.obfuscate_strings)
        ctx.printer(CODE_JSON__INDEX__HEADER_OBF);
    else
        ctx.printer(CODE_JSON__INDEX__HEADER);

    int i = 0;
    for (auto field : message.fields)
    {
        if (is_transient(field))
            continue;
        auto name = get_json_name(field);

        std::string label = ctx.number_names ? std::to_string(field.index) : name;
        if (ctx.obfuscate_strings)
            label = obfuscate(label);
        ctx.printer(CODE_JSON__INDEX__ITEM, label, i);
        ++i;
    }

    ctx.printer(CODE_JSON__INDEX__FOOTER);
}

static void generateModelWrapper( GeneratorContext &ctx, const Message &message )
{
    std::string typeName = nativePackage(message.package) + "::" + message.name + "_type";

    bool is_persistent = false;
    for (auto field : message.fields)
    {
        if (!is_transient(field))
        {
            is_persistent = true;
            break;
        }
    }

    ctx.printer(CODE_JSON_MODEL__HEADER, PROTOGEN_VERSION_NAMING, typeName);
    generate_function__read_field(ctx, message, typeName, is_persistent);
    generate_function__write(ctx, message, typeName, is_persistent);
    generate_function__empty(ctx, message, typeName);
    generate_function__clear(ctx, message, typeName);
    generate_function__equal(ctx, message, typeName);
    generate_function__swap(ctx, message, typeName);
    generate_function__index(ctx, message, is_persistent);
    ctx.printer(CODE_JSON_MODEL__FOOTER, PROTOGEN_VERSION_NAMING);
}

static void generateEntity( GeneratorContext &ctx, const Message &message )
{
    std::string originalType = nativePackage(message.package) + "::" + message.name + "_type";
    generateNamespace(ctx, message, true);
    ctx.printer(CODE_ENTITY, message.name , originalType, PROTOGEN_VERSION_NAMING);
    generateNamespace(ctx, message, false);
}

static void generateEntityWrapper( GeneratorContext &ctx, const Message &message )
{
    std::string typeName = nativePackage(message.package) + "::" + message.name;
    ctx.printer(CODE_ENTITY_JSON, typeName, typeName + "_type", PROTOGEN_VERSION_NAMING);
}

static void generateMessage( GeneratorContext &ctx, const Message &message )
{
    if (message.fields.size() > protogen::CppGenerator::MAX_FIELDS)
        throw protogen::exception("more than " + std::to_string(protogen::CppGenerator::MAX_FIELDS) +
            " fields in message '" + message.name + "'");

    ctx.printer("\n//\n// $1$\n//\n", message.name);

    // create the model structure
    generateModel(ctx, message);
    // create JSON wrapper for model structure
    generateModelWrapper(ctx, message);
    // create entoty structure and JSON wrapper
    generateEntity(ctx, message);
    generateEntityWrapper(ctx, message);
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

typedef std::vector<std::shared_ptr<protogen::Message>> MessageList;
typedef std::unordered_set<std::shared_ptr<protogen::Message>> MessageSet;

static bool contains( const MessageList &items, const std::shared_ptr<protogen::Message> &message )
{
    for (const auto &current : items)
        if (current == message)
            return true;
    return false;
}

static void sort( MessageList &output, MessageSet &pending, std::shared_ptr<protogen::Message> message )
{
    if (pending.count(message) > 0)
        throw exception("circular reference with '" + message->name + "'");
    if (contains(output, message))
        return; // already processed

    pending.insert(message);
    for (auto &field : message->fields)
    {
        if (field.type.ref != nullptr && !contains(output, field.type.ref))
            sort(output, pending, field.type.ref);
    }
    pending.erase(message);

    output.push_back(message);
}

static void sort( GeneratorContext &ctx )
{
    std::vector<std::shared_ptr<protogen::Message>> messages;
    std::unordered_set<std::shared_ptr<protogen::Message>> pending;
    for (auto &message : ctx.root.messages)
        sort(messages, pending, message);

    ctx.root.messages.swap(messages);
}

static void generateInclusions( GeneratorContext &ctx )
{
    bool has_array = false;
    bool has_base64 = false;
    bool has_string = false;
    bool has_number = false;

    // check what types of fields we have
    for (const auto &message : ctx.root.messages)
    {
        for (const auto &field : message->fields)
        {
            if (field.type.repeated)
            {
                if (field.type.id == protogen::TYPE_BYTES)
                    has_base64 = true;
                has_array = true;
            }
            if (field.type.id == protogen::TYPE_STRING)
                has_string = true;
            else
            if (field.type.id >= protogen::TYPE_DOUBLE && field.type.id <= protogen::TYPE_BOOL)
                has_number = true;

            if (has_array && has_base64 && has_string && has_number)
                break;
        }
    }
    // include the necessary headers
    ctx.printer(GENERATED__protogen_hh);
    ctx.printer(GENERATED__json_hh);
    if (has_array)
        ctx.printer(GENERATED__json_array_hh);
    if (has_base64)
        ctx.printer(GENERATED__json_base___hh);
    if (has_number)
        ctx.printer(GENERATED__json_number_hh);
    if (has_string)
        ctx.printer(GENERATED__json_string_hh);
}

static void generateModel( GeneratorContext &ctx )
{
    std::string guard = makeGuard(ctx.root.fileName);
    ctx.printer(CODE_HEADER, PROTOGEN_VERSION, ctx.root.fileName, guard);

    sort(ctx);

    // message declarations
    for (const auto &message : ctx.root.messages)
        generateMessage(ctx, *message);

    ctx.printer("#endif // $1$\n", guard);
}

void CppGenerator::generate( Proto3 &root, std::ostream &out )
{
    Printer printer(out);
    GeneratorContext ctx(printer, root);

    ctx.obfuscate_strings = get_option(ctx.root.options, PROTOGEN_O_OBFUSCATE_STRINGS, false);
    ctx.cpp_use_lists = get_option(ctx.root.options, PROTOGEN_O_CPP_USE_LISTS, false);
    ctx.number_names = get_option(ctx.root.options, PROTOGEN_O_NUMBER_NAMES, false);

    generateInclusions(ctx);
    generateModel(ctx);
}


}