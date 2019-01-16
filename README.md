# protogen

Tool to compile ``proto3`` schemas and generate C++ classes which serialize and deserialize JSON messages. The compiler generates a C++ header file to be included in your program. This file is all you need: no external libraries.

There is also the ``libprotogen_static`` library you can use to create your own compiler or enable your application to compile ``proto3``.

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
    int32 age = 2;
    repeated string colors = 3;
}
```

Compile ``.proto`` files using ``protogen`` program:

```
# ./protogen model.proto model.pg.hh
```

Include the generated header file in your source code to use the classe:

```c++
#include "model.pg.hh"

...

Person girl;
girl.name("Michelle");
girl.age(24);
girl.colors().push_back("yellow");
girl.serialize(std::cout);

```

Compile your program as usual (no additional library is required). In the example above, the output would be:

```
{"name":"Michelle","age":24,"colors":["yellow"]}
```

You can define `PROTOGEN_OBFUSCATE_NAMES` before the inclusion to enable obfuscated JSON field names. This feature can be used to make a little difficult for curious people to find out your JSON field names (e.g. internal microservices).

```
#define PROTOGEN_OBFUSCATE_NAMES
#include "model.pg.hh"
```

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
- [x] bytes
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
