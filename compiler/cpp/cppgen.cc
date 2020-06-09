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

static void generateVariable( GeneratorContext &ctx, const Field &field )
{
    std::string storage = fieldStorage(field);
    ctx.printer("$1$ $2$;\n", fieldNativeType(field, ctx.cpp_use_lists), storage, toUpper(storage), std::to_string(field.index));
}

static void generateCopyCtor( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("$1$(const $1$ &that) { *this = that; }\n", message.name);
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
        "bool operator!=(const $1$ &that) const { return !(*this == that); }\n"
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
        "PROTOGEN_NS::parse_result deserialize( PROTOGEN_NS::tokenizer &tok, bool required = false, PROTOGEN_NS::ErrorInfo *err = NULL ) {\n"
        "bool hfld[$1$] = {false};\n"
        "if (tok.next().id != PROTOGEN_NS::token_id::OBJS) PROTOGEN_REO(err, tok);\n"
        "while (true)\n{\n"
        "\tstd::string name;\nPROTOGEN_NS::token tt;\n"
        "tt = tok.next();\nif (tt.id == PROTOGEN_NS::token_id::OBJE) break;\n"
        "if (tt.id != PROTOGEN_NS::token_id::STRING) PROTOGEN_REN(err, tok);\n"
        "name.swap(tok.current().value);\n"
        "if (tok.next().id != PROTOGEN_NS::token_id::COLON) PROTOGEN_RES(err, tok);\n", message.fields.size());

    bool first = true;
    size_t count = 0;

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi, ++count)
    {
        if (!IS_VALID_TYPE(fi->type.id)) continue;
        if (fi->options.count(PROTOGEN_O_TRANSIENT))
        {
            OptionEntry opt = fi->options.at(PROTOGEN_O_TRANSIENT);
            if (opt.type == OptionType::BOOLEAN && opt.value == "true") continue;
        }

        std::string storage = fieldStorage(*fi);
        std::string type = nativeType(*fi);

        if (!first) ctx.printer("else\n");

        // start of the new field
        ctx.printer("// $1$\n", fi->name);
        ctx.printer("if (name == PROTOGEN_FN_$1$) {\n\t", storage); // open the main 'if'

        // 'repeated' and 'bytes'
        if (fi->type.repeated || fi->type.id == protogen::TYPE_BYTES)
        {
             const char *arrayType = "std::vector";
             if (ctx.cpp_use_lists && fi->type.id != protogen::TYPE_BYTES)
                 arrayType = "std::list";
             ctx.printer(
                 "if (PROTOGEN_NS::traits< $4$<$1$> >::read(tok, this->$2$, required, err) == PROTOGEN_NS::parse_result::ERROR) "
                 "PROTOGEN_REV(err, tok, name, \"$3$\");\n",
                 type, storage, proto3Type(*fi), arrayType);
        }
        else
        if (fi->type.id == protogen::TYPE_MESSAGE || fi->type.id == protogen::TYPE_STRING)
        {
            ctx.printer("this->$2$.clear();\n"
                "if (PROTOGEN_NS::traits<$1$>::read(tok, this->$2$, required, err) == PROTOGEN_NS::parse_result::ERROR) "
                "PROTOGEN_REV(err, tok, name, \"$3$\");\n", type, storage, proto3Type(*fi));
        }
        else
        {
            ctx.printer(
                "$1$ value;\n"
                "if (PROTOGEN_NS::traits<$1$>::read(tok, value, required, err) == PROTOGEN_NS::parse_result::ERROR) "
                "PROTOGEN_REV(err, tok, name, \"$3$\");\n"
                "this->$2$.swap(value);\n", type, storage, proto3Type(*fi));
        }
        ctx.printer(
            "hfld[$1$] = true;\n"
            "\b}\n", // closes the main 'if'
            count);
        first = false;
    }
    ctx.printer(
        "else\n"
        "// ignore the current field\n"
        "{\n\tif (PROTOGEN_NS::json::ignore_value(tok) == PROTOGEN_NS::parse_result::ERROR) PROTOGEN_REI(err, tok, name);\n"
        "\b}\n"
        "if (PROTOGEN_NS::json::next_field(tok) == PROTOGEN_NS::parse_result::ERROR) PROTOGEN_REI(err, tok, name);\n"
        "\b}\n");

    // check whether we missed any required field
    ctx.printer(
        "if (required) {\n\t");
    for (size_t i = 0, t = message.fields.size(); i < t; ++i)
    {
        if (message.fields[i].options.count(PROTOGEN_O_TRANSIENT))
        {
            OptionEntry opt = message.fields[i].options.at(PROTOGEN_O_TRANSIENT);
            if (opt.type == OptionType::BOOLEAN && opt.value == "true") continue;
        }
        ctx.printer("if (!hfld[$1$]) PROTOGEN_REM(err, tok, PROTOGEN_FN_$2$);\n", i, fieldStorage(message.fields[i]));
    }
    ctx.printer("\b}\nreturn PROTOGEN_NS::parse_result::OK;\n\b}\n");
}


static void generateSerializer( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        // serializer writing to 'ostream'
        "void serialize( PROTOGEN_NS::ostream &out ) const {\n"
        "\tout << '{';\n"
        "bool first = true;\n");

    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);
        if (fi->options.count(PROTOGEN_O_TRANSIENT))
        {
            OptionEntry opt = fi->options.at(PROTOGEN_O_TRANSIENT);
            if (opt.type == OptionType::BOOLEAN && opt.value == "true") continue;
        }

        if (fi->type.repeated || fi->type.id == protogen::TYPE_MESSAGE || fi->type.id == protogen::TYPE_STRING)
            ctx.printer(
                "// $1$\n"
                "if (!this->$1$.empty()) "
                "PROTOGEN_NS::json::write(out, first, PROTOGEN_FN_$1$, this->$1$);\n", storage);
        else
            ctx.printer(
                "// $1$\n"
                "if (!this->$1$.empty()) "
                "PROTOGEN_NS::json::write(out, first, PROTOGEN_FN_$1$, this->$1$());\n", storage);
    }
    ctx.printer("out << '}';\n\b}\n");
}


static void generateTrait( GeneratorContext &ctx, const Message &message )
{
    ctx.printer("PROTOGEN_TRAIT($1$::$2$)\n", nativePackage(message.package), message.name);
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

static void generateUndefined( GeneratorContext &ctx, const Message &message )
{
    ctx.printer(
        "bool empty() const {\n"
        "\treturn\n\t", message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        ctx.printer("this->$1$.empty()", fieldStorage(*fi));
        if (fi + 1 != message.fields.end())
            ctx.printer(" &&\n");
        else
            ctx.printer(";\n");
    }
    ctx.printer("\b\b}\n");
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

/*
    // begin namespace
    generateNamespace(ctx, message, true);

    std::string parent = "PROTOGEN_NS::message";
    if (!ctx.custom_parent.empty()) parent = nativePackage(ctx.custom_parent);


    ctx.printer("\nstruct $1$ : public $2$", message.name, parent);
    ctx.printer(
        " {\n"
        "\tstatic const uint32_t protogen_version = 0x$1$;\n", ctx.versionNo);

    // create the macro containing the field name
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        std::string storage = fieldStorage(*fi);
        std::string name;
        if (ctx.number_names)
        {
            name = std::to_string(fi->index);
        }
        else
            name = fieldStorage(*fi);

        if (obfuscate_strings)
        {
            ctx.printer(
                "#undef PROTOGEN_FN_$1$\n"
                "#define PROTOGEN_FN_$1$ PROTOGEN_NS::reveal(\"$2$\", $3$)\n",
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
    ctx.printer("$1$() {}\nvirtual ~$1$() {}\n", message.name);
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
    // use parent versions of 'serialize' and 'deserialize'
    //ctx.printer("using $1$::serialize;\nusing $1$::deserialize;\n", parent);
    // message serializer
    //generateSerializer(ctx, message);
    // message deserializer
    //generateDeserializer(ctx, message);
    // clear function
    //generateClear(ctx, message);
    // close class declaration
    ctx.printer("};\n");
    // end namespace
    generateNamespace(ctx, message, false);

    // JSON structure
    ctx.printer("PG_JSON($1$::$2$", nativePackage(message.package), message.name);
    for (auto fi = message.fields.begin(); fi != message.fields.end(); ++fi)
    {
        if (fi->options.count(PROTOGEN_O_TRANSIENT))
        {
            OptionEntry opt = fi->options.at(PROTOGEN_O_TRANSIENT);
            if (opt.type == OptionType::BOOLEAN && opt.value == "true") continue;
        }
        ctx.printer(", $1$", fi->name);
    }
    ctx.printer(");\n");

    // 'std::swap' specialization
    ctx.printer("namespace std { \n"
        "   template <typename T, typename std::enable_if<std::is_same<T,$1$::$2$>::value,int>::type = 0>\n"
        "   void swap( $1$::$2$& a, $1$::$2$& b ) noexcept { a.swap(b); }\n"
        "}\n",
        nativePackage(message.package), message.name);

    // message trait
    //generateTrait(ctx, message);
*/
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