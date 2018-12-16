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
            return field.typeName + "__type<0>";

        default:
            throw protogen::exception("Invalid field type");
    }
}


static void generateFunctions( std::ostream &out, const Field &field )
{
    std::string type = fieldNativeType(field);
    std::string name = toLower(field.name);

    bool ref = true;

    // constant getter
    out << "\tconst " << type << ((ref) ? "& " : " ") << name << "() const { return ";
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << name << "__; }\n";
    // mutable getter
    out << "\t" << type << ((ref) ? "& " : " ") << name << "() { return ";
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << name << "__; }\n";
    // setter
    out << "\tvoid " << name << "(const " << type << " &value) {";
    if (field.type == protogen::TYPE_MESSAGE) out << '*';
    out << name << "__ = value; }";

    out << '\n';
}


static void generateDeclarations( std::ostream &out, const Field &field )
{
    std::string type = fieldNativeType(field);
    std::string name = toLower(field.name);
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
    out << ' ' << field.name << "__;\n";
    // flags variable
    out << "\tuint32_t " << field.name << "__flags;\n";
}

static void generateMessage( std::ostream &out, const Message &message )
{
    std::string className = message.name + "__type";

    out << "template<int> class " << className << " {\nprivate:\n";

    for (auto it = message.fields.begin(); it != message.fields.end(); ++it)
    {
        // storage variable
        generateVariable(out, *it);
    }
    out << "public:\n";

    // default constructor
    out << '\t' << className << "() {}\n";
    // copy constructor
    out << '\t' << className << "(const " << className << " &obj) {}\n";
    // move constructor
    out << "#if __cplusplus >= 201103L\n";
    out << '\t' << className << "(const " << className << " &&obj) {}\n";
    out << "#endif\n";
    // destructor
    out << "\tvirtual ~" << className << "() {}\n";
    // assign operator
    out << '\t' << className << " &operator=(const " << className << " &obj) { return *this; }\n";
    // equality operator
    out << "\tbool operator==(const " << className << " &obj) { return false; }\n\n";

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
        // accessor functions
        generateFunctions(out, *it);
        out << '\n';
    }
    out << "};\n";//typedef " << message.name << "__type<0> " << message.name << ";\n\n";
}

static void generateModel( std::ostream &out, const Proto3 &proto )
{
    out << "// AUTO-GENERATED FILE, DO NOT EDIT!" << std::endl;
    out << "#ifndef GUARD_HH\n";
    out << "#define GUARD_HH\n\n";
    out << "#include <string>\n";
    out << "#include <stdint.h>\n\n";

    // forward declarations
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
    {
        out << "template<int> class " << it->name << "__type;\n";
    }

    // message declarations
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
        generateMessage(out, *it);

    // public types
    for (auto it = proto.messages.begin(); it != proto.messages.end(); ++it)
        out << "class " << it->name << " : public " << it->name << "__type<0> { };\n";

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