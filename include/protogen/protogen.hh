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

}

#endif // PROTOGEN_API_H