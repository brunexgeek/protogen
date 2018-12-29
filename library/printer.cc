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

#include "printer.hh"
#include <cstring>
#include <cstdlib>

namespace protogen {


Printer::Printer( std::ostream &out, bool pretty ) : out_(out), pretty_(pretty),
    newLine_(true), tab_(0)
{
}


void Printer::print( const char *format )
{
    out_ << format;
}


static void indent( std::ostream &out, int level )
{
    if (level < 1 || level > 20) return;
    switch (level)
    {
        case 1:  out << '\t'; break;
        case 2:  out << "\t\t"; break;
        case 3:  out << "\t\t\t"; break;
        case 4:  out << "\t\t\t\t"; break;
        case 5:  out << "\t\t\t\t\t"; break;
        case 6:  out << "\t\t\t\t\t\t"; break;
        case 7:  out << "\t\t\t\t\t\t\t"; break;
        default:
            for (int i = 0; i < level; ++i) out << '\t';
    }
}


void Printer::print( const char *format, const std::vector<std::string> &vars )
{
    //std::cerr << "vars has " << vars.size() << std::endl;
    const char *ptr = format;
    while (*ptr != 0)
    {
        if (*ptr == '$')
        {
            const char *end = strchr(ptr + 1, '$');
            if (end == nullptr) return;

            char *last = nullptr;
            int index = (int) strtol(ptr + 1, &last, 10) - 1;
            if (last != end) return;

            ptr = end + 1;
            //std::cerr << index << "  " << vars[0] << std::endl;;
            if (index < 0 || index >= (int) vars.size()) continue;

            if (newLine_)
            {
                indent(out_, tab_);
                newLine_ = false;
            }
            out_ << vars[index];
            continue;
        }
        if (*ptr == '\t' && newLine_)
        {
            if (tab_ < 7) ++tab_;
        }
        else
        if (*ptr == '\b' && newLine_)
        {
            if (tab_ > 0) --tab_ ;
        }
        else
        if (*ptr == '\n')
        {
            newLine_ = true;
            out_ << '\n';
        }
        else
        {
            if (newLine_)
            {
                indent(out_, tab_);
                newLine_ = false;
            }
            out_ << *ptr;
        }
        ++ptr;
    }
}


}