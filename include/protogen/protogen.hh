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

#ifndef PROTOGEN_API_H
#define PROTOGEN_API_H


#include <protogen/proto3.hh>


namespace protogen {


// Obfuscate generated strings (true) or keep them in plain text (false).
// The default value is false.
#define PROTOGEN_O_OBFUSCATE_STRINGS       "obfuscate_strings"

// Use field numbers as JSON field names (true) or use the actual names (false).
// The default value value is false.
#define PROTOGEN_O_NUMBER_NAMES            "number_names"

// Enable (true) or disable (false) the use of a parent class. When enabled, every
// message will specialize the 'Message' class which contains a virtual destructor and
// a couple of pure virtual functions. If you do not need a common ancestor for your
// message classes, you can disable this option.
#define PROTOGEN_O_CPP_ENABLE_PARENT       "cpp_enable_parent"

// Enable (true) or disable (false) information about parsing errors. The default value is false.
// If enabled, 'deserializer' functions will populate the 'ErrorInfo' object given as argument.
#define PROTOGEN_O_CPP_ENABLE_ERRORS       "cpp_enable_errors"


class Generator
{
    public:
        virtual void generate( Proto3 &proto, std::ostream &out ) = 0;
};


class CppGenerator : public Generator
{
    public:
        void generate( Proto3 &proto, std::ostream &out );
};

}

#endif // PROTOGEN_API_H