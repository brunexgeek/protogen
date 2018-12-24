#include <protogen/proto3.hh>
#include <protogen/protogen.hh>
#include <fstream>


int main( int argc, char **argv )
{
    if (argc != 3)
    {
        std::cerr << "Usage: protogen <proto3 file> <output file>\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream input(argv[1]);
    std::ofstream output(argv[2], std::ios_base::ate);

    if (input.good() && output.good())
    {
        protogen::Proto3 proto;
        protogen::Proto3::parse(proto, input, argv[1]);
        protogen::CppGenerator gen;
        gen.generate(proto, output);
    }
    return 0;
}