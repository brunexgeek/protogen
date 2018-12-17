# protogen

Experimental tool to compile ``proto3`` schemas and generate C++ classes which serialize and deserialize JSON messages.

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
}
```

Compile ``.proto`` files using ``protogen`` program:

```
# ./protogen < model.proto > model.hh
```

Include the generated header file in your source code to use the classes:

```c++
#include "model.hh"

...

Person girl;
girl.name = "Michelle";
girl.age = 24;
// serialization
girl.serialze(std::cout);
```

## Features

Supported output programming languages:
- [x] C++ (1998/2011)
- [ ] C99
- [ ] Java

Supported field types:
- [x] other messages (see [Limitations](#Limitations))
- [ ] repeated
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

Proto3 features:
- [x] Line comments
- [ ] Block comments
- [ ] Options

## Limitations

- You can't import additional ``.proto`` files;
- Circular references are not supported
- Messages used by other messages *must* be declared first;
