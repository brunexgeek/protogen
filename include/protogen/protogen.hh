/*
 * Copyright 2021 Bruno Ribeiro <https://github.com/brunexgeek>
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

// Make a field transient (true) or not (false). The default value is false.
#define PROTOGEN_O_TRANSIENT               "transient"

// Enable (true) or disable (false) the use of 'std::list' as container class for repeated
// fields. If disabled, 'std::vector' will be used instead. The default value is false.
#define PROTOGEN_O_CPP_USE_LISTS           "cpp_use_lists"

// Specify a custom name for the JSON field, while retaining the C++ field name as defined in the message.
// If no custom name is provided, the JSON field and the C++ field name will be the same.
#define PROTOGEN_O_NAME                    "name"

class Generator
{
    public:
        virtual void generate( Proto3 &proto, std::ostream &out ) = 0;
};


class CppGenerator : public Generator
{
    public:
        static const int MAX_FIELDS = sizeof(uint64_t) * 8;
        void generate( Proto3 &proto, std::ostream &out );
};

}

#endif // PROTOGEN_API_H