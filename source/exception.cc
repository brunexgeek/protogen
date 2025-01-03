/*
 * Copyright 2020-2024 Bruno Ribeiro <https://github.com/brunexgeek>
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

#include <protogen/exception.hh>
#include <sstream>

namespace protogen {


exception::exception( const std::string &message ) : message(message)
{
}

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