# protogen

Experimental tool to compile ``proto3`` schemas and generate C++ classes which serialize and deserialize JSON messages.

## Build

```
mkdir build && cd build && cmake ..
make
```

## Features

Supported output programming languages:
- [x] C++98
- [ ] C99
- [ ] Java

Supported field types:
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

