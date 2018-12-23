#include <protogen/proto3.hh>
#include <protogen/protogen.hh>
#include <sstream>



int main( int argc, char **argv )
{
    std::istream *input = &std::cin;
    std::stringstream text;
    if (argc == 2)
    {
        text << argv[1];
        input = &text;
    }
    protogen::Proto3 proto;
    protogen::Proto3::parse(*input, proto);
    protogen::CppGenerator gen;
    gen.generate(proto, std::cout);
    return 0;
}