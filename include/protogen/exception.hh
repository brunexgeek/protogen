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

#ifndef PROTOGEN_EXCEPTION_HH
#define PROTOGEN_EXCEPTION_HH


#include <string>

namespace protogen {

class Token;

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