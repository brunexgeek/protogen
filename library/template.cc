#include <fstream>
#include <iostream>

void process( std::istream &input, std::ostream &output )
{
    std::string line;
    bool content = false;

    output << "#ifndef CODE\n";
    output << "#define CODE\n";

    while (!input.eof())
    {
        std::getline(input, line);

        if (!content)
        {
            if (line.find("--- ") != 0) continue;
            line = line.substr(4);
            output << "#define " << line << " \\\n";
            content = true;
        }
        else
        {
            if (line.find("------") == 0)
            {
                output << ";\n";
                content = false;
                continue;
            }
            output << "    \"";
            for (auto it = line.begin(); it != line.end(); ++it)
            {
                if (*it == '"')
                    output << "\\\"";
                else
                if (*it == '\\')
                    output << "\\\\";
                else
                    output << *it;
            }
            output << "\\n\" \\\n";
        }
    }

    output << "#endif // CODE\n";
}


int main( int argc, char **argv )
{
    if (argc != 3) return 1;
std::cout << argv[1] << "  " << argv[2] << "----------\n";
    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    std::ofstream output(argv[2]);
    if (!output.good()) return 1;

    process(input, output);

    return 0;
}