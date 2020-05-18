# Tref: A *T*iny compile time *ref*lection system.

## Features
- Only utilize C++17 language features, no external preprocessor needed.
- Reflect at compile time with minimal runtime overhead.
- Simple syntax.
- Normal class reflection.
- Class template reflection.
- Reflect element with additional meta data.
- Enum class reflection, support user defined value and meta for each item.
- Reflect external types of third-party code.
- Reflect class level and instance level variables and functions.
- Reflect nested member types.
- Reflect overloaded functions.
- Factory pattern: introspect all sub-classes from one base class.

## TODO
- Reflect function details, e.g. arguments and return type.
- Specify a new name for reflected element.

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