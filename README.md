# Tref: A *T*iny compile time *ref*lection system.

## Motivation
I am a C++ game developer, I have been hunting for an easy to use reflection library for a long time, but none of them make me happy completely. 
What I need is a reflection system that:

- Reflect the field name. (magic_get excluded)
    - Needed by JSON reader & writer.
- Reflect subclass to support factory pattern. (none have direct support)
    - Games need to deserialize objects from game assets heavily.
    - The plugin system needs to create concrete plugin implementation to be registered into the game engine.
- Should have simple syntax for both normal class and class template. (almost all are weak at supporting class template or with complex syntax)    
    - The RPC system of the server may have a huge number of types of data to transmit, we need an elegant solution to make things easier.    
    - The RPC system can utilize the reflection info to encode the data into a more efficient format.
- Must support custom meta-data for most types of reflected elements. (all others are weak at supporting this)
    - The ORM system of the server needs meta for database related stuff like primary key, etc.
    - The game engine can choose parts of its API to be exported to the other system (e.g. the script engine) by specifying some special tags.
- Must support enum with custom values. (some library support this but with complex syntax)
    - No one can guarantee all enum are started from zero and increased by one.
- Better to support meta for enum items. (almost none support this)
    - Error codes defined as enum can have localization info attacked for translation.
    - You can even attach a function to the meta of enum item for data-driven like static-dispatching pattern.
- No extra preprocessor tools for the building system. (I hate struggling with building system)
    - Extra tools usually slow down the building system under the hood.
- Easily support reflecting 3rd-party code. (Other libraries are weak at supporting this)
    - A large project definitely integrates many 3rd libraries, keeps the wrapper layer thin and small and programmers will thank you so much.
- Should not brings in code bloat and slow down the compiling too much. 
    - As a basic facility, this is very important.


## Features
- Simpler syntax than other reflection libraries.
- Only utilize C++17 language features, no external preprocessor needed.
- Reflect at compile time with minimal runtime overhead.
- Normal class reflection.
- Class template reflection with unified syntax.
- Reflect elements with additional meta-data.
- Enum class reflection, support user-defined value, and meta for each item.
- Reflect external types of third-party code with unified syntax.
- Reflect class level and instance-level variables and functions.
- Reflect nested member types.
- Reflect overloaded functions.
- Factory pattern support: introspect all sub-classes from one base class.

## TODO
- Reflect function details, e.g. arguments and return type.
- Specify a new name for the reflected element.
- STL support.

## Examples

- See TrefTest.cpp.

## Thanks
- https://woboq.com/blog/verdigris-implementation-tricks.html
- https://www.codeproject.com/Articles/1002895/Clean-Reflective-Enums-Cplusplus-Enum-to-String-wi

## License

The MIT License

```
Copyright (C) 2018 crazybie<soniced@sina.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```