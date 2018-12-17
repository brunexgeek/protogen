#include <protogen/proto3.hh>
#include <sstream>


using protogen::Proto3;
using protogen::Message;
using protogen::Field;


static std::string toLower( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'A' && output[i] <= 'Z') output[i] = (char)(output[i] + 32);

    return output;
}


static std::string toUpper( const std::string &value )
{
    std::string output = value;
    for (size_t i = 0, t = output.length(); i < t; ++i)
        if (output[i] >= 'a' && output[i] <= 'z') output[i] = (char)(output[i] - 32);

    return output;
}


static std::string fieldStorage( const Field &field )
{
    return toLower(field.name) + "_S";
}

static std::string fieldFlags( const Field &field )
{
    return toLower(field.name) + "_F";
}

static std::string fieldNativeType( const Field &field )
{
    switch (field.type)
    {
        case protogen::TYPE_DOUBLE:
            return "double";

        case protogen::TYPE_FLOAT:
            return "float";

        case protogen::TYPE_SINT32:
        case protogen::TYPE_SFIXED32:
        case protogen::TYPE_INT32:
            return "int32_t";

        case protogen::TYPE_SINT64:
        case protogen::TYPE_SFIXED64:
        case protogen::TYPE_INT64:
            return "int64_t";

        case protogen::TYPE_FIXED32:
        case protogen::TYPE_UINT32:
            return "uint32_t";

        case protogen::TYPE_FIXED64:
        case protogen::TYPE_UINT64:
            return "uint64_t";

        case protogen::TYPE_BOOL:
            return "bool";

        case protogen::TYPE_STRING:
            return "std::string";

        case protogen::TYPE_BYTES:
            return "std::string&";

        case protogen::TYPE_MESSAGE:
            return field.typeName;

        default:
            throw protogen::exception("Invalid field type");
    }
}


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


static void generateFunctions( std::ostream &out, const Field &field )
{
    std::string type = fieldNativeType(field);
    std::string name = toLower(field.name);
    std::string storage = fieldStorage(field);

    bool ref = true;

    // constant getter
    out << "\tconst " << type << ((ref) ? "& " : " ") << name << "() const { return ";
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << storage << "; }\n";

    // setter
    out << "\tvoid " << name << "(const " << type << " &value) {";
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << storage << " = value; }\n";

    // additional string setter
    if (field.type == protogen::TYPE_STRING)
    {
        out << "\tvoid " << name << "(const char *value) { " << storage << " = value; }\n";
    }

    // returns the field index
    out << "\tint index_" << name << "() const { return " << field.index << "; }\n";

    // returns the field name
    out << "\tstd::string name_" << name << "() const { return \"" << field.name << "\"; }\n";

    out << '\n';
}


static void generateDeclarations( std::ostream &out, const Field &field )
{
    std::string type = fieldNativeType(field);
    std::string name = fieldStorage(field);
    // constant getter
    out << "\tconst " << type << ((field.type == protogen::TYPE_STRING) ? "& " : " ") << name << "() const;\n";
    // mutable getter
    out << '\t' << type << " &" << name << "();\n";
    // setter
    out << "\tvoid " << name << "(const " << type << " value);\n";
}


static void generateVariable( std::ostream &out, const Field &field )
{
    // storage variable
    out << '\t' << fieldNativeType(field);
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << ' ' << fieldStorage(field) << ";\n";
    // flags variable
    out << "\tuint32_t " << fieldFlags(field) << ";\n";
}


static void generateCtor( std::ostream &out, const Message &message )
{
    out << '\t' << message.name << "() {\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        if (it->type == protogen::TYPE_STRING) continue;
        out << "\t\t" << fieldStorage(*it) << " = ";

        if (it->type == protogen::TYPE_MESSAGE)
            out << "new " << it->typeName << "();\n";
        else
            out << defaultValue(*it) << ";\n";
    }

    out << "\t}\n";
}


static void generateDtor( std::ostream &out, const Message &message )
{
    out << "\tvirtual ~" << message.name << "() {\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        if (it->type != protogen::TYPE_MESSAGE) continue;
        out << "\t\tdelete " << fieldStorage(*it) << ";\n";
    }

    out << "\t}\n";
}


static void generateHasField( std::ostream &out, const Field &field )
{
    out << "\tbool has_" << field.name << "() const { return " << fieldFlags(field) << " & 0x01; }\n";
}

static void generateClearField( std::ostream &out, const Field &field )
{
    out << "\tvoid clear_" << field.name << "() { " << fieldStorage(field) << " = ";

    if (field.type == protogen::TYPE_MESSAGE)
        out << "NULL; ";
    else
        out << defaultValue(field) << "; ";
    out << fieldFlags(field) << " &= ~0x01; }\n";
}


static void generateMessage( std::ostream &out, const Message &message )
{
    out << "\nclass " << message.name << " {\nprivate:\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        // storage variable
        generateVariable(out, *it);
    }
    out << "public:\n";

    // default constructor
    generateCtor(out, message);
    // copy constructor
    out << '\t' << message.name << "(const " << message.name << " &obj) {}\n";
    // move constructor
    out << "#if __cplusplus >= 201103L\n";
    out << '\t' << message.name << "(const " << message.name << " &&obj) {}\n";
    out << "#endif\n";
    // destructor
    generateDtor(out, message);
    // assign operator
    out << '\t' << message.name << " &operator=(const " << message.name << " &obj) { return *this; }\n";
    // equality operator
    out << "\tbool operator==(const " << message.name << " &obj) { return false; }\n\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {

        // constants with field information
        out << "#if __cplusplus >= 201103L\n";
        out << "\tstatic constexpr const int " << toUpper(it->name) << "_ID" << " = " << it->index << ";\n";
        out << "\tstatic constexpr const char *" << toUpper(it->name) << "_NAME = \"" << it->name << "\";\n";
        out << "#else\n";
        out << "\t#define " << toUpper(it->name) << "_ID" << "  " << it->index << "\n";
        out << "\t#define " << toUpper(it->name) << "_NAME  \"" << it->name << "\"\n";
        out << "#endif\n";
        // 'has' function
        generateHasField(out, *it);
        // 'clear' function
        generateClearField(out, *it);
        // accessor functions
        generateFunctions(out, *it);
        out << '\n';
    }
    out << "};\n";//typedef " << message.name << "__type<0> " << message.name << ";\n\n";
}


extern unsigned char ___library_picojson_hh[];
extern  unsigned int ___library_picojson_hh_len;


static void generateModel( std::ostream &out, const Proto3 &proto )
{
    out << "// AUTO-GENERATED FILE, DO NOT EDIT!" << std::endl;

    out.write((char*)___library_picojson_hh, ___library_picojson_hh_len);

    out << "\n\n#ifndef GUARD_HH\n";
    out << "#define GUARD_HH\n\n";
    out << "#include <string>\n";
    out << "#include <stdint.h>\n\n";

    // forward declarations
    out << "// forward declarations\n";
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    {
        out << "\nclass " << it->name << ";\n";
    }

    // message declarations
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
        generateMessage(out, *it);

    // public types
    //for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    //    out << "class " << it->name << " : public " << it->name << "__type<0> { };\n";

    out << "#endif\n";
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