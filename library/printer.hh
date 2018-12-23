#ifndef PROTOGEN_PRINTER_HH
#define PROTOGEN_PRINTER_HH


#include <iostream>
#include <vector>
#include <string>


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


        static std::string toString( const std::string& s ) { return s; }

        void print( const char *format, const std::vector<std::string> &vars );
};


}


#endif // PROTOGEN_PRINTER_HH