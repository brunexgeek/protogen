/*
 * Copyright 2018 Bruno Ribeiro <https://github.com/brunexgeek>
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

#include <protogen/proto3.hh>
#include <protogen/protogen.hh>
#include <fstream>
#include <limits.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#endif

void main_usage()
{
    std::cerr << "protogen " << PROTOGEN_VERSION << std::endl;
    std::cerr << "Usage: protogen <proto3 file> <output file>\n";
    exit(EXIT_FAILURE);
}


void main_error( const std::string &message )
{
    std::cerr << "ERROR: " << message << std::endl;
    exit(EXIT_FAILURE);
}


int main( int argc, char **argv )
{
    if (argc != 3) main_usage();

    std::ifstream input(argv[1]);
    if (!input.good()) main_error(std::string("Unable to open '") + argv[1] + "'");

    std::ofstream output(argv[2], std::ios_base::ate);
    if (!output.good()) main_error(std::string("Unable to open '") + argv[2] + "'");

    int result = 0;
    #ifdef _WIN32
	char fullPath[MAX_PATH] = { 0 };
	if (GetFullPathName(argv[1], MAX_PATH, fullPath, nullptr) == 0) return 1;
	#else
	char fullPath[PATH_MAX] = { 0 };
	if (realpath(argv[1], fullPath) == nullptr) return 1;
	#endif


    protogen::Proto3 proto;
    try
    {
        protogen::Proto3::parse(proto, input, fullPath);
        protogen::CppGenerator gen;
        gen.generate(proto, output);
    } catch (protogen::exception &ex)
    {
        std::cerr << fullPath << ':' << ex.line << ':' << ex.column << ": error: " << ex.cause() << std::endl;
        result = 1;
    }
    input.close();
    output.close();

    return result;
}