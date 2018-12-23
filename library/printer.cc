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


static const char *indent( int level )
{
    switch (level)
    {
        case 1: return "\t";
        case 2: return "\t\t";
        case 3: return "\t\t\t";
        case 4: return "\t\t\t\t";
        case 5: return "\t\t\t\t\t";
        case 6: return "\t\t\t\t\t\t";
        case 7: return "\t\t\t\t\t\t\t";
    }
    return "";
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
            if (index < 0 || index >= vars.size()) continue;

            if (newLine_)
            {
                out_ << indent(tab_);
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
                out_ << indent(tab_);
                newLine_ = false;
            }
            out_ << *ptr;
        }
        ++ptr;
    }
}


}