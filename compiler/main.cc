#include <protogen/proto3.hh>
#include <sstream>


using protogen::Proto3;
using protogen::Message;
using protogen::Field;


#define IS_VALID_TYPE(x)    ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_BYTES )
#define IS_NUMERIC_TYPE(x)  ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_SFIXED64 )


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


static std::string fieldStorage( const Field &field, bool qualified = true )
{
    if (!qualified)
        return toLower(field.name);
    else
        return "this->" + toLower(field.name);
}

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


// TODO: use 'protogen::traits' instead
static std::string defaultValue( const Field &field )
{
    switch (field.type)
    {
        case protogen::TYPE_DOUBLE:
            return "0.0";

        case protogen::TYPE_FLOAT:
            return "0.0F";

        case protogen::TYPE_SINT32:
        case protogen::TYPE_SFIXED32:
        case protogen::TYPE_INT32:
        case protogen::TYPE_SINT64:
        case protogen::TYPE_SFIXED64:
        case protogen::TYPE_INT64:
        case protogen::TYPE_FIXED32:
        case protogen::TYPE_UINT32:
        case protogen::TYPE_FIXED64:
        case protogen::TYPE_UINT64:
            return "0";

        case protogen::TYPE_BOOL:
            return "false";

        case protogen::TYPE_STRING:
            return "\"\"";

        case protogen::TYPE_BYTES:
            return "\"\"";

        default:
            throw protogen::exception("Unable to generate default value");
    }
}


static void generateVariable( std::ostream &out, const Field &field )
{
    // storage variable
    out << "\t" << fieldNativeType(field) << ' ' << fieldStorage(field, false) << ";\n";
}


static void generateCopyCtor( std::ostream &out, const Message &message )
{
    out << '\t' << message.name << "(const " << message.name << " &that) {\n";
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
        out << "\t\tthis->" << it->name << " = that." << it->name << ";\n";
    out << "\t}\n";
}


static void generateAssignOperator( std::ostream &out, const Message &message )
{
    out << '\t' << message.name << " &operator=(const " << message.name << " &that) {\n";
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
        out << "\t\tthis->" << it->name << " = that." << it->name << ";\n";
    out << "\t\treturn *this;\n\t}\n";
}


static void generateEqualityOperator( std::ostream &out, const Message &message )
{
    out << "\tbool operator==(const " << message.name << " &that) const {\n";
    out << "\t\treturn\n";
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        out << "\t\tthis->" << it->name << " == that." << it->name;
        if (it + 1 != message.fields.end())
            out << " &&\n";
        else
            out << ";\n";
    }
    out << "\t}\n";
}


static void generateRepeatedSerializer( std::ostream &out, const Field &field )
{
    std::string storage = fieldStorage(field);

    out << indent(2) << "if (" << storage << "().size() > 0) {\n";
    out << indent(3) << "bool first = true;\n";
    out << indent(3) << "out << \"\\\"" << field.name << "\\\" : [ \";\n";
    out << indent(3) << "for (std::vector<" << nativeType(field) << ">::const_iterator it = " << storage
        << "().begin(); it != " << storage << "().end(); ++it) {\n";
    out << indent(4) << "if (!first) out << \", \";\n";
    out << indent(4) << "first = false;\n";

    storage = "(*it)";

    // message fields
    if (field.type == protogen::TYPE_MESSAGE)
        out << indent(4) << storage << ".serialize(out);\n";
    else
    {
        // numerical field
        if (field.type >= protogen::TYPE_DOUBLE && field.type <= protogen::TYPE_SFIXED64)
            out << indent(4) << "out << " << storage << ";\n";
        else
        // string fields
        if (field.type == protogen::TYPE_STRING)
            out << indent(4) << "out << '\"' << " << storage << " << '\"';\n";
    }
    out << indent(3) << "}\n" << indent(3) << "out << \" ]\"; \n" << indent(2) << "};\n";
}


static void generateDeserializer( std::ostream &out, const Message &message )
{
    out << indent(1) << "void deserialize( const std::string &json ) {\n";
    out << indent(2) << "protogen::InputStream<std::string::const_iterator> in(json.begin(), json.end());\n";
    out << indent(2) << "if (in.get() != '{') return;\n";
    out << indent(2) << "std::string name;\n";
    out << indent(2) << "while (true) {\n";
    out << indent(3) << "name.clear();\n";
    out << indent(3) << "if (!protogen::JSON::readName(in, name)) break;\n";

    bool first = true;

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        if (!IS_VALID_TYPE(it->type) || it->repeated) continue;

        if (!first) out << " else\n";

        out << indent(3) << "if (name == \"" << it->name << "\") {\n";
        out << indent(4) << nativeType(*it) << " value;\n";

        // numerical field
        if (IS_NUMERIC_TYPE(it->type))
            out << indent(4) << "if (!protogen::JSON::read(in, value)) break;\n";
        else
        // string fields
        if (it->type == protogen::TYPE_STRING)
            out << indent(4) << "if (!protogen::JSON::read(in, value)) break;\n";

        out << indent(4) << fieldStorage(*it) << "(value);\n";
        out << indent(4) << "if (!protogen::JSON::next(in)) break;\n";
        out << indent(3) << "}";
        first = false;
    }
    out << indent(2) << "}}\n\n";
}


static void generateSerializer( std::ostream &out, const Message &message )
{
    out << indent(1) << "void serialize( std::ostream &out ) const {\n";
    out << indent(2) << "out << '{';\n";
    out << indent(2) << "bool first = true;\n";
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        out << indent(2) << "// " << it->name << '\n';
        if (it->repeated)
        {
            generateRepeatedSerializer(out, *it);
            continue;
        }
        std::string storage = fieldStorage(*it) + "()";

        out << indent(2);
        if (it->type != protogen::TYPE_MESSAGE)
            out << "if (!" << fieldStorage(*it) << ".undefined()) ";
        out << "protogen::JSON::write(out, first, \"" << it->name << "\", " << storage << ");\n";

        if (it->repeated) out << "}\n";
    }
    out << indent(2) << "out << '}';\n";
    out << indent(1) << "}\n";
}

static void generateTrait( std::ostream &out, const Message &message )
{
    out << "namespace protogen {\ntemplate<> struct traits<" << message.name << "> {\n";
    out << "\tstatic bool isMessage() { return true; }\n";
    out << "\tstatic bool isNumber() { return false; }\n";
    out << "\tstatic bool isString() { return false; }\n";
    out << "\tstatic void clear( " << message.name << " &value ) { }\n";
    out << "}; }";
}

static void generateClear( std::ostream &out, const Message &message )
{
    out << "\tvoid clear() {\n";
    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        if (it->repeated)
            out << "\t\tthis->" << it->name << "().clear();\n";
        else
        if (it->type == protogen::TYPE_MESSAGE)
            out << "\t\tthis->" << it->name << ".clear();\n";
        else
            out << "\t\tthis->" << it->name << "(" << defaultValue(*it) << ");\n";
    }
    out << "\t}\n";
}

static void generateMessage( std::ostream &out, const Message &message )
{
    generateTrait(out, message);

    out << "\nclass " << message.name << " : public protogen::Message {\npublic:\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        // storage variable
        generateVariable(out, *it);
    }

    // default constructor
    out << '\t' << message.name << "() {}\n";
    // copy constructor
    generateCopyCtor(out, message);
    #if 0
    // move constructor
    out << "#if __cplusplus >= 201103L\n";
    out << '\t' << message.name << "(const " << message.name << " &&obj) {}\n";
    out << "#endif\n";
    #endif
    // assign operator
    generateAssignOperator(out, message);
    // equality operator
    generateEqualityOperator(out, message);
    // message serializer
    generateSerializer(out, message);
    // message deserializer
    generateDeserializer(out, message);
    // clear function
    generateClear(out, message);

    out << "};\n";
}


extern const char *BASE_TEMPLATE;


static void generateModel( std::ostream &out, const Proto3 &proto )
{
    out << "// AUTO-GENERATED FILE, DO NOT EDIT!" << std::endl;

    out << "\n\n#ifndef GUARD_HH\n";
    out << "#define GUARD_HH\n\n";
    out << "#include <string>\n";
    out << "#include <stdint.h>\n\n";

    // base template
    out << '\n' << BASE_TEMPLATE << '\n';
    // forward declarations
    out << "\n// forward declarations\n";
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    {
        out << "\nclass " << it->name << ";\n";
    }
    // message declarations
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
        generateMessage(out, *it);
    out << "#endif // GUARD_HH\n";
}


int main( int argc, char **argv )
{
    std::istream *input = &std::cin;
    std::stringstream text;
    if (argc == 2)
    {
        text << argv[1];
        input = &text;
    }
    protogen::Proto3 *proto = protogen::Proto3::parse(*input);
    if (proto != nullptr)
        generateModel(std::cout, *proto);
    return 0;
}