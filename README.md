# Tref: A *T*iny compile-time *ref*lection system.

## Motivation
I am a C++ game developer, I have been hunting for a powerful and easy to use reflection library for a long time, but none of them make me happy completely(even the new proposal). 

What I need is a reflection system that:

#### Must be efficient for performance critical scenarios.
- Must not be as slow as the implementation of C# or Java or Golang.
- Should utilize the powerful compiling-time-executing feature of C++.

#### Reflect the field name. (magic_get excluded)
- Needed by JSON reader & writer.

#### Reflect subclass to support factory pattern. (none have direct support)
- Games need to deserialize objects from game assets heavily.
- The plugin system needs to create concrete plugin implementation to be registered into the game engine.

#### Should have simple syntax for both normal class and class template. (almost all are weak at supporting class template or with complex syntax)    
- The RPC system of the server may have a huge number of types of data to transmit, we need an elegant solution to make things easier.    
- The RPC system can utilize the reflection info to encode the data into a more efficient format.
- Good template-support is a bonus for better code-reuse.

#### Must support custom meta-data for most types of reflected elements. (all others are weak at supporting this)
- The ORM system of the server needs meta for database related attributes like primary key, unique keys, etc.
- The game engine can export part of its API to other system (e.g. the script engine) by specifying tags.
- Allow better visual-editing for the exported game structures in the engine editor.

#### Must support enum with custom values. (some library support this but with complex syntax)
- No one can guarantee all enums are started from zero and increased by one.

#### Better to support meta for enum items. (almost none support this)
- Error codes defined as enum can have localization info attacked for translation.
- You can even attach a function to the meta of enum item for data-driven like static-dispatching pattern.

#### Easily support reflecting 3rd-party code. (Other libraries are weak at supporting this)
- A large project definitely integrates many 3rd libraries, keeps the wrapper layer thin and small and programmers will thank you so much.

#### No extra preprocessor tools for the building system. (I hate struggling with building system)
- Extra tools usually slow down the building system under the hood.

#### Should not brings in code bloat and slow down the compiling too much. 
- This is very important to a basic facility.
    
So for summary, systems and design that are already known to strongly benefit from reflection are:
- Factory pattern.
- Data-driven design.
- Serialization system.
- RPC system.
- ORM system.
- Scripting system.
- Visual-Editing authoring tools.
and more.

## Features
- Simpler syntax than other reflection libraries.
- Only utilize C++17 language features, no external preprocessor tools needed.
- Super lightweight: only one small header file with no extra dependencies except STL.
- Reflect at compile time with minimal runtime overhead.
- Normal class and class template reflection with unified syntax.
- Reflect elements with additional meta-data.
- Enum class reflection, support user-defined value, and meta for each item.
- Reflect external types of third-party code.
- Reflect class-level and instance-level variables and functions.
- Reflect nested member types.
- Reflect overloaded functions.
- Factory pattern support: introspect all sub-classes from one base class.

## Tested Platforms
- MSVC 2017 (conformance mode & non-conformance mode)
- Clang 10

## TODO
- Reflect function details, e.g. arguments and return type.
- Specify a new name for the reflected element.
- STL support.

## Examples

- simple class
```c++
struct TypeA {
  TrefType(TypeA);

  int val;
  TrefField(val);
};

static_assert(is_reflected_v<TypeA>);
static_assert(class_info<TypeA>().name == "TypeA");
static_assert(class_info<TypeA>().size == sizeof(TypeA));
static_assert(class_info<TypeA>().each_field([](auto info, int) {
  using mem_t = decltype(info.value);
  return info.name == "val" && is_same_v<enclosing_class_t<mem_t>, TypeA> &&
         is_same_v<member_t<mem_t>, decltype(TypeA{}.val)>;
}));
static_assert(class_info<TypeA>().get_field_index("val") == 1);
static_assert(class_info<TypeA>().get_field<1>().index == 1);
static_assert(class_info<TypeA>().get_field<1>().name == "val");
static_assert(
    is_same_v<decltype(class_info<TypeA>().get_field<1>())::member_t, int>);    
    
```

- subclass
```c++

struct TypeB : TypeA {
  TrefType(TypeB);

  float foo;
  TrefField(foo);
};

static_assert(has_base_class_v<TypeB>);
static_assert(is_same_v<base_of_t<TypeB>, TypeA>);
static_assert(is_same_v<decltype(class_info<TypeB>())::base_t, TypeA>);
static_assert(class_info<TypeB>().each_field([](auto info, int level) {
  // exclude members of base class
  if (level != 0)
    return true;

  using mem_t = decltype(info.value);
  return info.name == "foo" && is_same_v<enclosing_class_t<mem_t>, TypeB> &&
         is_same_v<member_t<mem_t>, decltype(TypeB{}.foo)>;
}));
```

- template subclass
```c++
template <typename T>
struct TempType : TypeB {
  TrefType(TempType);

  T tempVal;
  TrefField(tempVal);
};

static_assert(class_info<TempType<int>>().name == "TempType");
static_assert(class_info<TempType<int>>().each_field([](auto info, int lv) {
  // exclude members of base class.
  if (lv != 0)
    return true;
  using mem_t = decltype(info.value);
  return info.name == "tempVal" &&
         is_same_v<enclosing_class_t<mem_t>, TempType<int>> &&
         is_same_v<member_t<mem_t>, int>;
}));
```

- class meta
```c++
struct FakeMeta {
  int foo;
  float bar;
};

// A computed meta
constexpr int makeMetaFoo(int a, int b) {
  return a + b;
}

struct ClassWithMeta {
  TrefTypeWithMeta(ClassWithMeta, (FakeMeta{makeMetaFoo(1, 2), 22}));
};
static_assert(class_info<ClassWithMeta>().meta.foo == 3);
static_assert(class_info<ClassWithMeta>().meta.bar == 22);
```

- function overloading
```c++
struct OverloadingTest {
  TrefType(OverloadingTest);

  template <int, int>
  struct Meta {};

  void foo(int);
  void foo(float);
  void foo(char*, int);

  // Can omit the paren for function with only one argument.
  TrefField(foo, int);  // equals to TreField(foo, (int))

  TrefFieldWithMeta(foo, (float), (Meta<1, 2>{}));

  TrefField(foo, (char*, int));
};

static_assert(class_info<OverloadingTest>().each_field([](auto info, int) {
  if (info.name != "foo")
    return false;
  if constexpr (is_same_v<decltype(info.value),
                          void (OverloadingTest::*)(int)>) {
    return info.value == overload_v<int>(&OverloadingTest::foo);

  } else if constexpr (is_same_v<decltype(info.value),
                                 void (OverloadingTest::*)(float)>) {
    static_assert(is_same_v<decltype(info.meta), OverloadingTest::Meta<1, 2>>);
    return info.value == overload_v<float>(&OverloadingTest::foo);

  } else if constexpr (is_same_v<decltype(info.value),
                                 void (OverloadingTest::*)(char*, int)>) {
    return info.value == overload_v<char*, int>(&OverloadingTest::foo);
  }
  return false;
}));
```

- global enum
```c++
struct FakeEnumMeta {
  int foo;
  int bar;
};

TrefEnumWithMeta(EnumA,
                 (FakeEnumMeta{111, 222}),
                 Ass = 1,
                 Ban = (int)EnumA::Ass * 3);

static_assert(enum_to_string(EnumA::Ass) == "Ass");
static_assert(string_to_enum("Ban", EnumA::Ass) == EnumA::Ban);
static_assert(enum_info<EnumA>().meta.foo == 111);
static_assert(enum_info<EnumA>().meta.bar == 222);
static_assert(enum_info<EnumA>().name == "EnumA");
static_assert(enum_info<EnumA>().size == sizeof(EnumA));
static_assert(enum_info<EnumA>().items.size() == 2);
static_assert(enum_info<EnumA>().each_item([](auto info) {
  switch (info.value) {
    case EnumA::Ass:
      return info.name == "Ass";
    case EnumA::Ban:
      return info.name == "Ban";
    default:
      return false;
  }
}));
```

- external enum
```c++
enum class ExternalEnum { Value1 = 1, Value2 = Value1 + 4 };

TrefExternalEnum(ExternalEnum, Value1, Value2);

static_assert(enum_info<ExternalEnum>().name == "ExternalEnum");
static_assert(enum_info<ExternalEnum>().size == sizeof(ExternalEnum));
static_assert(enum_info<ExternalEnum>().items.size() == 2);
```

- meta for enum values
```c++
struct CustomEnumItem {
  string_view desc;
  string_view comment;
  int otherMetaData = 0;
};

TrefEnumEx(EnumValueMetaTest,
           (TestA,
            (CustomEnumItem{"Desc for A Test", "Comment for A Test", 11})),
           (TestB,
            (CustomEnumItem{"Desc for B Test", "Comment for B Test", 22})));

static_assert(enum_info<EnumValueMetaTest>().items[0].meta.desc ==
              "Desc for A Test");
static_assert(enum_info<EnumValueMetaTest>().items[0].meta.comment ==
              "Comment for A Test");
static_assert(enum_info<EnumValueMetaTest>().items[0].meta.otherMetaData == 11);

static_assert(enum_info<EnumValueMetaTest>().items[1].meta.desc ==
              "Desc for B Test");
static_assert(enum_info<EnumValueMetaTest>().items[1].meta.comment ==
              "Comment for B Test");
static_assert(enum_info<EnumValueMetaTest>().items[1].meta.otherMetaData == 22);
```

- static dispatching sample.
```c++
enum class TestEnumStaticDispatching;
constexpr auto processA(TestEnumStaticDispatching v) {
  return 111;
}
constexpr auto processB(TestEnumStaticDispatching v) {
  return 222;
}

TrefEnumEx(TestEnumStaticDispatching, (EnumA, &processA), (EnumB, &processB));

static_assert([] {
  constexpr auto c = TestEnumStaticDispatching::EnumA;
  constexpr auto idx = enum_info<TestEnumStaticDispatching>().index_of_value(c);
  return enum_info<TestEnumStaticDispatching>().items[idx].meta(c) == 111;
}());
```

- factory pattern
```c++

struct Base {
  TrefType(Base);

  int baseVal;
  TrefField(baseVal);
};

template <typename T>
struct Data : Base {
  TrefType(Data);

  T t;
  TrefFieldWithMeta(t, Meta{"test"});

  int x, y;
  TrefFieldWithMeta(x, (MetaNumber{"pos x", 1, 100}));
  TrefFieldWithMeta(y, (MetaNumber{"pos y", 1, 100}));

  std::string name{"boo"};
  TrefFieldWithMeta(name, Meta{"entity name"});
};

struct Child : Data<int> {
  TrefType(Child);

  float z;
  TrefField(z);
};
TrefSubType(Data<int>);
TrefSubType(Child);


template <typename T>
constexpr bool hasSubclass(string_view name) {
  auto found = false;
  class_info<T>().each_subclass([&](auto info, int) {
    found = info.name == name;
    return !found;
  });
  return found;
}

static_assert(hasSubclass<Base>("Child"));


template <class T>
void dumpTree() {
  printf("===== All Subclass of %s ====\n", class_info<T>().name.data());

  class_info<T>().each_subclass([&](auto info, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");

    assert(info.name.size() > 0);
    printf("%s (rtti: %s)\n", info.name.data(),
           typeid(decltype(info)::class_t).name());
    return true;
  });
  puts("============");
}
```

- deserialize from file
```c++

template <typename T,
          typename = std::enable_if_t<tref::is_reflected_v<T>, bool>>
bool operator>>(JsonReader& r, T& d) {
  if (!r.expectObjStart())
    return false;

  for (std::string key; !r.expectObjEnd();) {
    if (!r.expectObjKey(key))
      return false;

    auto loaded = false;
    tref::class_info<T>().each_field([&](auto info, int) {
      if (info.name == key) {
        auto ptr = info.value;
        if (r >> d.*ptr) {
          loaded = true;
        } else {
          r.onInvalidValue(key.c_str());
        }
        return false;
      }
      return true;
    });
    if (!loaded) {
      r.onInvalidValue(key.c_str());
      return false;
    }
  }
  return true;
}

template <typename T>
bool operator>>(JsonReader& in, std::unique_ptr<T>& p) {
  std::string typeName;
  if (!(in >> typeName)) {
    in.onInvalidValue(typeName.c_str());
    return false;
  }

  auto loaded = false;
  tref::class_info<T>().each_subclass([&](auto info, int) {
    if (typeName == info.name) {
      using C = typename decltype(info)::class_t;
      auto i = std::make_unique<C>();
      if (in >> *i) {
        loaded = true;
        p = std::move(i);
      } else {
        in.onInvalidValue(typeName.c_str());
        loaded = false;
      }
      return false;
    }
    return true;
  });
  return loaded;
}

```

- check TrefTest.cpp for more examples.

## Thanks To
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
