#include <fstream>
#include <iostream>

void process( std::istream &input, std::ostream &output, const std::string &guard )
{
    std::string line;
    bool content = false;

    output << "#ifndef " << guard << '\n';
    output << "#define " << guard << '\n';

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
                output << "\n";
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

    output << "#endif // " << guard << '\n';
}


int main( int argc, char **argv )
{
    if (argc != 3) return 1;

    std::cout << "Processing '"<< argv[1] << "' to generate '" << argv[2] << "'" << std::endl;

    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    std::ofstream output(argv[2]);
    if (!output.good()) return 1;

    std::string guard = argv[1];
    #ifdef _WIN32
    const char DIR_SEPARATOR = '\\';
    #else
    const char DIR_SEPARATOR = '/';
    #endif
    auto pos = guard.rfind(DIR_SEPARATOR);
    if (pos != std::string::npos) guard = guard.substr(pos);
    guard = "GENERATED_" + guard;
    for (auto it = guard.begin(); it++ != guard.end();)
        if (!isalpha(*it)) *it = '_';

    process(input, output, guard);

    return 0;
}