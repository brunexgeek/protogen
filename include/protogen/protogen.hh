#ifndef PROTOGEN_API_H
#define PROTOGEN_API_H


#include <protogen/proto3.hh>


namespace protogen {

class Generator
{
    public:
        virtual void generate( Proto3 &proto, std::ostream &out, const std::string &fileName ) = 0;
};


class CppGenerator : public Generator
{
    public:
        void generate( Proto3 &proto, std::ostream &out, const std::string &fileName = "" );
};

}

#endif // PROTOGEN_API_H