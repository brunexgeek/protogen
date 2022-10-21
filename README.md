# protogen [![Build Status](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fbrunexgeek%2Fprotogen%2Fbadge%3Fref%3Dmaster&label=build&logo=none)](https://actions-badge.atrox.dev/brunexgeek/protogen/goto?ref=master)

Protogen enables you to serialize data as JSON in C++ programs. To use the library you only need a C++11 capable compiler (e.g. Visual Studio or GCC). No extenal dependencies are required.

## Build

```
# mkdir build && cd build
# cmake ..
# make
```

## Usage

Protogen can be used in two ways:
* As a serialization mechanism for existing types;
* To compile 'proto3' files and generate serializable types.

### Serialization of existing types

Use the macro ``PG_JSON`` to create the internal types used to serialize data. The macro parameters are the type name and the public fields you want to be able to serialize. The type can have up to 24 serializable fields. Make sure the ``PG_JSON`` call is in the global scope (i.e. outside any namespace).

```c++
struct MyData
{
    std::string name;
    int age;
    std::list<std::string> pets;
};

PG_JSON(MyData, name, age, pets)

```

Now it's possible to serialize and deserialize the type using the functions ``serialize`` and ``deserialize``.

```c++
using protogen_2_0_2;

MyData obj;
obj.name = "Margot";
obj.age = 30;
obj.pets.push_back("Rufus");
// serialize
std::string json;
serialize(book, json);
// deserialize
MyData tmp;
deserialize(tmp, json);
```

### Compiling 'proto3' files

Create a ``.proto`` file:

```
syntax = "proto3";

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

Include ``protogen.hh`` and the generated header file in your source code. It's important to include ``protogen.hh`` before any use of the generated header file, otherwise a compilation error will be issued.

It's also important to match the version of ``protogen.hh`` with the version used to generate the types. You can mix multiple versions of protogen in the same project.

```c++
#include "protogen.hh"
#include "model.pg.hh"

...

// create and populate an object
Person girl;
girl.name = "Michelle";
girl.age = 24;
girl.colors.push_back("yellow");

// JSON serialization
std::string json;
girl.serialize(json);

// JSON deserialization with error information (optional)
Person person;
Person::ErrorInfo err;
if (!person.deserialize(json, false, &err))
    std::cerr << "Error: " << err.message << " at " << err.line << ':' << err.column << std::endl;
std::cout << girl.name << std::endl;

...
```

Compile the program as usual. In the example above, the output would be:

```
{"name":"Michelle","age":24,"colors":["yellow"]}
Michelle
```

Types generated by protogen compiler contain helper functions like ``serialize`` and comparison operators. However, you can still use functions from protogen namespace with those types.

## Supported options

These options can be set in the proto3 file:

* **obfuscate_strings** (top-level) &ndash; Enable string obfuscation. If enabled, all strings in the C++ generated file will be obfuscated with a very simple (and insecure) algorithm. The default value is `false`. This option can be used to make a little difficult for curious people to find out your JSON field names by inspecting binary files.
* **number_names** (top-level) &ndash; Use field numbers as JSON field names. The default value is `false`. If enabled, every JSON field name will be the number of the corresponding field in the `.proto` file. This can significantly reduce the size of the JSON output.
* **transient** (field-level) &ndash; Make the field transient (`true`) or not (`false`). Transient fields are not serialized/deserialized. The default value is `false`.
* **cpp_use_lists** (top-level) &ndash; Use `std::list` (`true`) instead of `std::vector` (`false`) in repeated fields. This gives best performance if your program constantly changes repeated fields (add and/or remove items). This option does not affect `bytes` fields which always use `std::vector`. The default value is `false`.

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
- [x] Options
- [ ] Nested messages
- [ ] Enumerations

## Limitations

These are the current limitations of the implementation. Some of them may be removed in future versions.

Proto3 parser:
- You cannot import additional ``.proto`` files;
- Circular references are not supported;

JSON parser:
- Strings do not support ``\u`` to specify unicode endpoints;

## License

The library and the compiler are distributed under [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).

Code generated by **protogen** compiler is distributed under [The Unlicense](http://unlicense.org), but it depends on ``protogen.hh`` which is licensed by [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).