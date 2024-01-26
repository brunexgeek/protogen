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
#include <unordered_set>
#include <vector>
#include <cmake.hh>
#include <auto-code.hh>
#include <auto-protogen.hh>
#include <auto-json.hh>
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
    bool cpp_enable_errors = false;
    bool cpp_use_lists = false;
    std::string custom_parent;

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
    { protogen::TYPE_STRING,   "string",    "std::string" , "\"\"" },
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
        ctx.printer("\t\t$1$ $2$;\n", fieldNativeType(field, ctx.cpp_use_lists), fieldStorage(field) );
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

static void generateModelWrapper( GeneratorContext &ctx, const Message &message )
{
    std::string typeName = nativePackage(message.package) + "::" + message.name + "_type";
    std::stringstream temp_deserialize_if;
    std::stringstream temp_serialize_if;
    std::stringstream temp_empty_if;
    std::stringstream temp_clear_call;
    std::stringstream temp_equal_if;
    std::stringstream temp_swap_call;
    std::stringstream temp_is_missing_if;
    int i = 0;
    for (auto field : message.fields)
    {
        std::string name = field.name;
        std::string label = name;
        if (ctx.number_names) label = std::to_string(field.index);

        if (ctx.obfuscate_strings)
        {
            auto len = label.length();
            label = obfuscate(label);
            label = Printer::format("reveal(\"$1$\",$2$)", label, len);
        }
        else
            label = Printer::format("\"$1$\"", label);

        bool transient = false;
        auto it = field.options.find(PROTOGEN_O_TRANSIENT);
        if (it != field.options.end())
            transient = it->second.type == OptionType::BOOLEAN && it->second.value == "true";

        if (!transient)
        {
            Printer::format(temp_deserialize_if, CODE_DESERIALIZE_IF, name, label, i);
            Printer::format(temp_serialize_if, CODE_SERIALIZE_IF, name, label);
            Printer::format(temp_is_missing_if, CODE_IS_MISSING_IF, label, 1 << i);
        }
        Printer::format(temp_empty_if, CODE_EMPTY_IF, name);
        Printer::format(temp_clear_call, CODE_CLEAR_CALL, name);
        Printer::format(temp_equal_if, CODE_EQUAL_IF, name);
        Printer::format(temp_swap_call, CODE_SWAP_CALL, name);
        ++i;
    }

    ctx.printer(CODE_JSON_MODEL, typeName,
        temp_deserialize_if.str(),
        temp_serialize_if.str(),
        temp_empty_if.str(),
        temp_clear_call.str(),
        temp_equal_if.str(),
        temp_swap_call.str(),
        temp_is_missing_if.str(),
        PROTOGEN_VERSION_NAMING);
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
    if (message.fields.size() > 24)
        throw exception(std::string("Message '") + message.name + "' have more than 24 fields");

    ctx.printer("\n//\n// $1$\n//\n", message.name);

    // create the model structure
    generateModel(ctx, message);
    // create JSON wrapper for model structure
    generateModelWrapper(ctx, message);
    // create entoty structure and JSON wrapper
    //ctx.printer("PG_JSON_ENTITY($1$, $2$)", message.name, message.name + "_type");
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
    ctx.printer(GENERATED__protogen_hh);
    ctx.printer(GENERATED__json_hh);
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

    generateInclusions(ctx);
    generateModel(ctx);
}


}