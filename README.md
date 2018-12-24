# protogen

Experimental tool to compile ``proto3`` schemas and generate C++ classes which serialize and deserialize JSON messages.

The compiler generates a C++ header file to be included in your program. This file is all you need: no external libraries.

## Build

```
# mkdir build && cd build && cmake ..
# make
```

## Usage

Create a ``.proto`` file:

```
message Person {
    string name = 1;
    int age = 2;
    repeated string colors = 3;
}
```

Compile ``.proto`` files using ``protogen`` program:

```
# ./protogen model.proto model.hh
```

Include the generated header file in your source code to use the classes:

```c++
#include "model.hh"

...

Person girl;
girl.name("Michelle");
girl.age(24);
girl.colors().push_back("yellow");
girl.serialize(std::cout);

...

girl.deserialize(std::cin);
```

Compile your program as usual (no additional library is required).

## Features

Supported field types:
- [x] other messages (see [Limitations](#Limitations))
- [x] repeated
- [x] double
- [x] float
- [x] int32
- [x] int64
- [x] uint32
- [x] uint64
- [x] sint32
- [x] sint64
- [x] fixed32
- [x] fixed64
- [x] sfixed32
- [x] sfixed64
- [x] bool
- [x] string
- [ ] bytes
- [ ] any
- [ ] oneof
- [ ] maps

Proto3 syntax features:
- [x] Line comments
- [x] Block comments
- [x] Packages
- [ ] Imports
- [ ] Options

## Limitations

These are the current limitations of the implementation. Some of them may be removed in future versions.

Proto3 parser:
- You cannot import additional ``.proto`` files;
- Circular references are not supported;
- Messages used by other messages *must* be declared first;

JSON parser:
- Strings do not support ``\u`` to specify unicode endpoints;
- ``null`` values are not recognized.
