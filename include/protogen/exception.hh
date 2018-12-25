#ifndef PROTOGEN_EXCEPTION_HH
#define PROTOGEN_EXCEPTION_HH


#include <string>

namespace protogen {

class exception : public std::exception
{
    public:
        int line, column;

        exception( const std::string &message, int line = 1, int column = 1 );
        virtual ~exception();
        const char *what() const throw();
        const std::string cause() const;
    private:
        std::string message;
};

} // namespace protogen


#endif // PROTOGEN_EXCEPTION_HH