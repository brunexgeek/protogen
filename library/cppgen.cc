#include <protogen/protogen.hh>
#include "printer.hh"


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
    { true,  protogen::TYPE_BYTES,    "bytes",     "std::string" , "\"\"" },
    { false, protogen::TYPE_MESSAGE,  nullptr,     nullptr       , nullptr }
};


static const char *indent( int level = 1 )
{
    switch (level)
    {
        case 1: return "\t";
        case 2: return "\t\t";
        case 3: return "\t\t\t";
        case 4: return "\t\t\t\t";
        case 5: return "\t\t\t\t\t";
    }
    return "";
}


static std::string toLower( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'A' && output[i] <= 'Z') output[i] = (char)(output[i] + 32);

    return output;
}

#if 0
static std::string toUpper( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'a' && output[i] <= 'z') output[i] = (char)(output[i] - 32);

    return output;
}
#endif


static std::string nativePackage( const std::string &package, bool append = true )
{
    if (package.empty()) return "";

    std::string name;

    for (auto it = package.begin(); it != package.end(); ++it)
        if (*it == '.')
            name += "::";
        else
            name += *it;
    if (append) name += "::";
    return name;
}


static std::string fieldStorage( const Field &field, bool qualified = true )
{
    if (!qualified)
        return toLower(field.name);
    else
        return "this->" + toLower(field.name);
}


/**
 * Translates protobuf3 types to C++ types.
 */
static std::string nativeType( const Field &field )
{

    if (field.type >= protogen::TYPE_DOUBLE && field.type <= protogen::TYPE_BYTES)
    {
        int index = (int)field.type - (int)protogen::TYPE_DOUBLE;
        return TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type == protogen::TYPE_MESSAGE)
        return field.typeName;
    else
        throw protogen::exception("Invalid field type");
}

/**
 * Translates protobuf3 types to protogen types.
 */
static std::string fieldNativeType( const Field &field )
{
    std::string output = "protogen::";

    if (field.repeated)
        output += "RepeatedField<";
    else
        output += "Field<";

    if (field.type >= protogen::TYPE_DOUBLE && field.type <= protogen::TYPE_BYTES)
    {
        int index = (int)field.type - (int)protogen::TYPE_DOUBLE;
        output += TYPE_MAPPING[index].nativeType;
    }
    else
    if (field.type == protogen::TYPE_MESSAGE)
        output += field.typeName;
    else
        throw protogen::exception("Invalid field type");

    output += '>';
    return output;
}


static void generateVariable( Printer &printer, const Field &field )
{
    // storage variable
    printer("$1$ $2$;\n", fieldNativeType(field),fieldStorage(field, false));
}


static void generateCopyCtor( Printer &printer, const Message &message )
{
    printer("$1$(const $1$ &that) {\n\t", message.name);
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
        printer("this->$1$ = that.$1$;\n", it->name);
    printer("\b}\n");
}


static void generateMoveCtor( Printer &printer, const Message &message )
{
    printer(
        "#if __cplusplus >= 201103L\n"
        "$1$($1$ &&that) {\n\t", message.name);
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        //if (it->repeated || it->type == protogen::TYPE_MESSAGE)
            printer("this->$1$ = std::move(that.$1$);\n", it->name);
        //else
        //    printer("this->$1$ = that.$1$;\n", it->name);
    }
    printer("\b}\n#endif\n");
}


static void generateAssignOperator( Printer &printer, const Message &message )
{
    printer("$1$ &operator=(const $1$ &that) {\n\t", message.name);
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
        printer("this->$1$ = that.$1$;\n", it->name);
    printer("return *this;\n\b}\n");
}


static void generateEqualityOperator( Printer &printer, const Message &message )
{
    printer(
        "bool operator==(const $1$&that) const {\n"
        "\treturn\n\t", message.name);
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        printer("this->$1$ == that.$1$", it->name);
        if (it + 1 != message.fields.end())
            printer(" &&\n");
        else
            printer(";\n");
    }
    printer("\b\b}\n");
}


static void generateDeserializer( Printer &printer, const Message &message )
{
    printer(
        "bool deserialize( std::istream &in ) {\n"
        "\tbool skip = in.flags() & std::ios_base::skipws;\n"
        "std::noskipws(in);\n"
        "std::istream_iterator<char> itb(in);\n"
        "std::istream_iterator<char> ite;\n"
        "protogen::InputStream< std::istream_iterator<char> > is(itb, ite);\n"
        "bool result = this->deserialize(is);\n"
        "if (skip) std::skipws(in);\n"
        "\treturn result;\n"
        "\b\b}\n"

        "template<typename T>\n"
        "bool deserialize( protogen::InputStream<T> &in ) {\n"
        "\tin.skip_ws();\n"
        "if (in.get() != '{') return false;\n"
        "std::string name;\n"
        "while (true) {\n\t"
        "name.clear();\n"
        "if (!protogen::json::readName(in, name)) break;\n");

    bool first = true;

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        if (!IS_VALID_TYPE(it->type)) continue;

        if (!first) printer("else\n");
        printer("if (name == \"$1$\") {\n\t", it->name);

        if (it->repeated)
        {
            std::string function = "readArray";
            if (it->type == protogen::TYPE_MESSAGE) function = "readMessageArray";
            printer("if (!protogen::json::$1$(in, $2$())) return false;\n", function, fieldStorage(*it));
        }
        else
        {
            if (it->type >= protogen::TYPE_DOUBLE && it->type <= protogen::TYPE_BYTES)
            {
                printer(
                    "$1$ value;\n"
                    "if (!protogen::json::readValue(in, value)) return false;\n"
                    "$2$(value);\n", nativeType(*it), fieldStorage(*it));
            }
            else
            if (it->type == protogen::TYPE_MESSAGE)
                printer("if (!$1$().deserialize(in)) return false;\n", fieldStorage(*it));
        }
        printer("if (!protogen::json::next(in)) return false;\n");
        printer("\b}\n"); // closes the main 'if'
        first = false;
    }
    printer(
        "else\n"
        "\tif (!protogen::json::ignore(in)) return false;\n\b\b"
        "}\nreturn true;\n\b}\n\n");
}


static void generateRepeatedSerializer( Printer &printer, const Field &field )
{
    std::string storage = fieldStorage(field);

    printer(
        "if ($1$().size() > 0) {\n\t"
        // this 'first' variable is from 'generateSerializer'
        "if (!first) { out << \", \"; first = false; };\n"
        // redeclaring 'first'
        "bool first = true;\n"
        "out << \"\\\"$2$\\\" : [ \";\n"
        "for (std::vector<$3$>::const_iterator it = $1$().begin(); it != $1$().end(); ++it) {\n"
        "\tif (!first) out << \", \";\n"
        "first = false;\n", storage, field.name, nativeType(field));

    storage = "(*it)";

    // message fields
    if (field.type == protogen::TYPE_MESSAGE)
        printer("$1$.serialize(out);\n", storage);
    else
    {
        // numerical field
        if (field.type >= protogen::TYPE_DOUBLE && field.type <= protogen::TYPE_SFIXED64)
            printer("out << $1$;\n", storage);
        else
        // string fields
        if (field.type == protogen::TYPE_STRING)
            printer("out << '\"' << $1$ << '\"';\n", storage);
    }
    printer("\b}\nout << \" ]\"; \n\b};\n");
}


static void generateSerializer( Printer &printer, const Message &message )
{
    printer(
        "void serialize( std::ostream &out ) const {\n"
        "\tout << '{';\n"
        "bool first = true;\n");
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        printer("// $1$\n", it->name);
        if (it->repeated)
        {
            generateRepeatedSerializer(printer, *it);
            continue;
        }
        std::string storage = fieldStorage(*it) + "()";

        if (it->type != protogen::TYPE_MESSAGE)
            printer("if (!$1$.undefined()) ", fieldStorage(*it));

        printer("protogen::json::write$1$(out, first, \"$2$\", $3$);\n",
            (it->type == protogen::TYPE_MESSAGE) ? "Message" : "",
            it->name, storage);
    }
    printer("out << '}';\n\b}\n");
}

static void generateTrait( Printer &printer, const Message &message )
{
    printer(
        "namespace protogen {\n"
        "\ttemplate<> struct traits<$1$$2$> {\n"
        "\tstatic bool isMessage() { return true; }\n"
        "static bool isNumber() { return false; }\n"
        "static bool isString() { return false; }\n"
        "static void clear( $1$$2$ &value ) { (void) value; }\n"
        "\b};\n\b}\n", nativePackage(message.package), message.name);
}

static void generateClear( Printer &printer, const Message &message )
{
    printer("void clear() {\n\t");
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        printer("this->$1$$2$.clear();\n", it->name, (it->repeated) ? "()" : "");
    }
    printer("\b}\n");
}

static void generateMessage( Printer &printer, const Message &message )
{
    generateTrait(printer, message);

    if (!message.package.empty())
        printer("namespace $1$ {\n", nativePackage(message.package, false));

    printer(
        "\nclass $1$ : public protogen::Message {\npublic:\n\t", message.name);

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        // storage variable
        generateVariable(printer, *it);
    }

    // default constructor
    printer("$1$() {}\n", message.name);
    // copy constructor
    generateCopyCtor(printer, message);
    // move constructor
    generateMoveCtor(printer, message);
    // assign operator
    generateAssignOperator(printer, message);
    // equality operator
    generateEqualityOperator(printer, message);
    // message serializer
    generateSerializer(printer, message);
    // message deserializer
    generateDeserializer(printer, message);
    // clear function
    generateClear(printer, message);
    // close class declaration
    printer("\b};\n");

    if (!message.package.empty())
        printer("} // namespace $1$\n", message.package);
}


static void generateModel( Printer &printer, const Proto3 &proto, const std::string &fileName )
{
    printer(
        "// Generated by the protogen $1$ compiler.  DO NOT EDIT!\n"
        "// Source: $3$\n"
        "\n#ifndef GUARD_HH\n"
        "#define GUARD_HH\n\n"
        "#include <string>\n"
        "#include <stdint.h>\n"
        "#include <iterator>\n\n"
        // base template
        "$2$\n", PROTOGEN_VERSION, BASE_TEMPLATE, fileName);

    // forward declarations
    printer("\n// forward declarations\n");
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    {
        if (!it->package.empty())
            printer("namespace $1$ { class $2$; }\n", nativePackage(it->package, false), it->name);
        else
            printer("class $1$;\n", it->name);
    }
    // message declarations
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
        generateMessage(printer, *it);

    printer("#endif // GUARD_HH\n");
}


void CppGenerator::generate( Proto3 &proto, std::ostream &out, const std::string &fileName )
{
    Printer printer(out);
    generateModel(printer, proto, fileName);
}


}