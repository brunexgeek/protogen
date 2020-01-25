/*
 * Copyright 2020 Bruno Ribeiro <https://github.com/brunexgeek>
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

#ifndef PROTOGEN_PRINTER_HH
#define PROTOGEN_PRINTER_HH


#include <iostream>
#include <vector>
#include <string>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace protogen {


class Printer
{
    public:
        Printer( std::ostream &out, bool pretty = false );

        void print( const char *format );

        template <typename... T>
        void operator()(const char* format, const T&... args)
        {
            print(format, {toString(args)...});
        }

        std::ostream &output() { return out_; }

    protected:
        std::ostream &out_;
        bool pretty_;
        bool newLine_;
        int tab_;

        static std::string toString( const std::string& value ) { return value; }
        static std::string toString( const size_t value )
        {
            char temp[24] = { 0 };
            snprintf(temp, sizeof(temp) - 1, "%lu", value);
            return std::string(temp);
        }

        static std::string toString( const ssize_t value )
        {
            char temp[24] = { 0 };
            snprintf(temp, sizeof(temp) - 1, "%ld", value);
            return std::string(temp);
        }

        void print( const char *format, const std::vector<std::string> &vars );
};


}


#endif // PROTOGEN_PRINTER_HH