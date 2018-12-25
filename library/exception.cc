#include <protogen/exception.hh>
#include <sstream>


namespace protogen {


exception::exception( const std::string &message, int line, int column ) :
    line(line), column(column), message(message)
{
}

exception::~exception()
{
}

const char *exception::what() const throw()
{
    return message.c_str();
}

const std::string exception::cause() const
{
    std::stringstream ss;
    ss << message << " (" << line << ':' << column << ')';
    return ss.str();
}


} // namespace protogen