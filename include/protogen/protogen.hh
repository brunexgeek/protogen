#ifndef PROTOGEN_API_H
#define PROTOGEN_API_H


#include <protogen/proto3.hh>


namespace protogen {

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

};

#endif // PROTOGEN_API_H