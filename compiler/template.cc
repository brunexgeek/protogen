#include <fstream>
#include <iostream>
#include <string>
#include <cmake.hh>

void copy( std::istream &input, std::ostream &output, const std::string &name )
{
    std::string line;

    output << "#define " << name << " \\\n";
    while (!input.eof())
    {
        std::getline(input, line);
		if (!input.good()) break;

        if (line.length() > 0 && line[0] == '#' && line.find("AUTO-REMOVE") != std::string::npos)
            continue;

        auto pos = line.find("_X_Y_Z");
        if (pos != std::string::npos)
            line.replace(pos, 6, PROTOGEN_VERSION_NAMING);

        output << "    \"";
        for (auto c : line)
        {
            if (c == '"')
                output << "\\\"";
            else
            if (c == '\\')
                output << "\\\\";
            else
                output << c;
        }
        output << "\\n\" \\\n";
    }
    output << "    \"\\n\"\n";
}

void process( std::istream &input, std::ostream &output, const std::string &guard )
{
    std::string line;
    bool content = false;

    output << "#ifndef " << guard << '\n';
    output << "#define " << guard << '\n';

    while (!input.eof())
    {
        std::getline(input, line);
		if (!input.good()) break;

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

static bool ends_with(const std::string &text, const std::string &needle)
{
    auto pos = text.rfind(needle);
    return pos != std::string::npos && pos + needle.length() == text.length();
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
    if (pos != std::string::npos)
        guard = guard.substr(pos+1);
    guard = "GENERATED__" + guard;
    for (auto it = guard.begin(); it != guard.end(); ++it)
        if (!isalpha(*it)) *it = '_';

    if (ends_with(argv[1], ".hh"))
        copy(input, output, guard);
    else
        process(input, output, guard);


    return 0;
}