#include "tref.h"

#include <functional>
#include <iostream>
#include <sstream>

using namespace std;
using namespace tref;

//////////////////////////////////////////////////////////////////////////
// basic usage

//////////////////////////////
// simple class

struct TypeA {
  TrefType(TypeA);

  int val;
  TrefField(val);
};

static_assert(is_reflected<TypeA>());
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

//////////////////////////////
// subclass

struct TypeB : TypeA {
  TrefType(TypeB);

  float foo;
  TrefField(foo);
};

static_assert(has_base_class<TypeB>());
static_assert(is_same_v<TrefBaseClass(TypeB), TypeA>);
static_assert(is_same_v<decltype(class_info<TypeB>())::base_t, TypeA>);
static_assert(class_info<TypeB>().each_field([](auto info, int level) {
  // exclude members of base class
  if (level != 0)
    return true;

  using mem_t = decltype(info.value);
  return info.name == "foo" && is_same_v<enclosing_class_t<mem_t>, TypeB> &&
         is_same_v<member_t<mem_t>, decltype(TypeB{}.foo)>;
}));
static_assert(class_info<TypeB>().each_field([](auto info, int lv) {
  if (lv == 0)
    return info.name == "foo";
  return lv == 1 && info.name == "val";
}));

//////////////////////////////
// template subclass

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

struct SubTypeA : TempType<int> {
  TrefType(SubTypeA);
};
static_assert(is_same_v<TrefBaseClass(SubTypeA), TempType<int>>);
static_assert(!is_same_v<TrefBaseClass(SubTypeA), TempType<float>>);

struct SubTypeB : TempType<float> {
  TrefType(SubTypeB);
};

static_assert(is_same_v<TrefBaseClass(SubTypeB), TempType<float>>);
static_assert(!is_same_v<TrefBaseClass(SubTypeB), TempType<int>>);

//////////////////////////////
// class meta

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

//////////////////////////////
// external template class as member

template <typename A, typename B>
struct ReflectedTemplate {
  TrefType(ReflectedTemplate);

  A a;
  TrefField(a);
};

struct UsingTemplateIntance {
  TrefType(UsingTemplateIntance);

  ReflectedTemplate<int, float> a;
  TrefField(a);
};

static_assert(class_info<decltype(UsingTemplateIntance{}.a)>().name ==
              "ReflectedTemplate");
static_assert(class_info<UsingTemplateIntance>().each_field([](auto info, int) {
  using MT = decltype(info)::member_t;
  return class_info<MT>().name == "ReflectedTemplate";
}));

//////////////////////////////

struct TestIsMember {
  TrefType(TestIsMember);

  static constexpr auto staticVal = 0;
  TrefField(staticVal);

  int memberVal;
  TrefField(memberVal);
};

static_assert(class_info<TestIsMember>().each_field([](auto info, int) {
  if (info.name == "staticVal")
    return !info.is_member_v;
  if (info.name == "memberVal")
    return info.is_member_v;
  return false;
}));

//////////////////////////////////////////
// Test template instance as macro argument

template <typename, typename>
struct MetaMultiArgs {};

struct TestMultipleArgsToMacro {
  TrefTypeWithMeta(TestMultipleArgsToMacro, (MetaMultiArgs<int, float>{}));

  template <typename A, typename B>
  static constexpr auto MultiArgsVar_v = A(0) + B(1);

  TrefFieldWithMeta((MultiArgsVar_v<int, int>), (MetaMultiArgs<float, char>{}));

  template <typename, typename, typename>
  struct T {};

  TrefMemberTypeWithMeta((T<int, float, string>),
                         (MetaMultiArgs<float, char>{}));
};

//////////////////////////////////

struct TestInnerTemplate {
  TrefType(TestInnerTemplate);

  template <typename C>
  struct InnerTemplate {
    TrefType(InnerTemplate);

    int a;
    TrefField(a);
  };

  TrefMemberType(InnerTemplate<int>);
};

static_assert(class_info<TestInnerTemplate::InnerTemplate<int>>().name ==
              "InnerTemplate");

static_assert(class_info<TestInnerTemplate::InnerTemplate<int>>().each_field(
    [](auto info, int) {
      static_assert(is_same_v<decltype(info)::member_t, int>);
      return info.name == "a";
    }));

static_assert(class_info<TestInnerTemplate>().each_member_type([](auto info,
                                                                  int) {
  using MT = decltype(info.value)::type;

  static_assert(is_same_v<MT, TestInnerTemplate::InnerTemplate<int>>);
  static_assert(class_info<MT>().name == "InnerTemplate");

  return info.name == "InnerTemplate<int>";
}));

//////////////////////////////////
// function overloading

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

//////////////////////////////////////////////////////////////////////////
// enum test

// global enum

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

// external enum

enum class ExternalEnum { Value1 = 1, Value2 = Value1 + 4 };

TrefEnumImp(ExternalEnum, Value1, Value2);

static_assert(enum_info<ExternalEnum>().name == "ExternalEnum");
static_assert(enum_info<ExternalEnum>().size == sizeof(ExternalEnum));
static_assert(enum_info<ExternalEnum>().items.size() == 2);

// enum in template (both name and value contains comma)

template <typename, typename>
struct TestExternalTemplateInnerEnum {
  enum class InnerEnum { ValX = 1, ValY = ValX + 10 };
};

// enum value is not necessary to external enum, here just for test.
TrefEnumImp(
    (TestExternalTemplateInnerEnum<int, int>::InnerEnum),
    ValX = 1,
    (ValY = (int)TestExternalTemplateInnerEnum<int, int>::InnerEnum::ValX +
            10));

static_assert(
    enum_info<TestExternalTemplateInnerEnum<int, int>::InnerEnum>().name ==
    "TestExternalTemplateInnerEnum<int, int>::InnerEnum");
static_assert(enum_info<TestExternalTemplateInnerEnum<int, int>::InnerEnum>()
                  .items.size() == 2);
static_assert(
    enum_to_string(TestExternalTemplateInnerEnum<int, int>::InnerEnum::ValX) ==
    "ValX");
static_assert(
    string_to_enum("ValX",
                   TestExternalTemplateInnerEnum<int, int>::InnerEnum::ValY) ==
    TestExternalTemplateInnerEnum<int, int>::InnerEnum::ValX);

// enum in class

struct DataWithEnumMemType {
  TrefType(DataWithEnumMemType);

  TrefMemberEnum(EnumF, ValA = 1, ValB = 12);
  TrefMemberType(EnumF);
};

static_assert(class_info<DataWithEnumMemType>().each_member_type([](auto info,
                                                                    int) {
  using T = decltype(info.value)::type;
  static_assert(is_enum_v<T> && is_same_v<T, DataWithEnumMemType::EnumF>);
  return info.name == "EnumF";
}));

//////////////////////////////////
// meta for enum values

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

//////////////////////////////////
// meta for values of enum in class

struct TestInnerEnumValueWithMeta {
  TrefMemberEnumEx(EnumValueWithMeta, (EnumA, "Enum A"), (EnumB, "Enum B"));

  TrefMemberEnumEx(EnumValueWithMeta2,
                   (EnumA, (CustomEnumItem{"desc for a", "comment for a", 11})),
                   (EnumB,
                    (CustomEnumItem{"desc for b", "comment for b", 22})));
};

static_assert(
    enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta>().items[0].meta ==
    "Enum A");
static_assert(
    enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta>().items[1].meta ==
    "Enum B");

static_assert(enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta2>()
                  .items[0]
                  .meta.desc == "desc for a");
static_assert(enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta2>()
                  .items[0]
                  .meta.otherMetaData == 11);
static_assert(enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta2>()
                  .items[1]
                  .meta.desc == "desc for b");
static_assert(enum_info<TestInnerEnumValueWithMeta::EnumValueWithMeta2>()
                  .items[1]
                  .meta.otherMetaData == 22);

//////////////////////////////////////////////////////////////////////////

template <typename T>
void DumpEnum() {
  printf("========= Enum Members of %s ======\n", enum_info<T>().name.data());
  enum_info<T>().each_item([](auto info) {
    printf("name: %.*s, val: %d\n", info.name.size(), info.name.data(),
           (int)info.value);
    return true;
  });
  puts("==================");
}

void TestEnum() {
  DumpEnum<EnumA>();
  DumpEnum<ExternalEnum>();
}

//////////////////////////////////////////////////////////////////////////
// reflection of hierarchy with custom meta

template <typename T>
constexpr bool hasSubclass(string_view name) {
  auto found = false;
  class_info<T>().each_subclass([&](auto info, int) {
    found = info.name == name;
    return !found;
  });
  return found;
}

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

template <typename T>
void dumpDetails() {
  constexpr auto& clsInfo = class_info<T>();
  printf("==== subclass details of %s ====\n", clsInfo.name.data());

  constexpr auto memName = "baseVal";
  constexpr auto index = clsInfo.get_field_index(memName);
  printf("index of %s: %d\n", memName,
         index > 0 ? clsInfo.get_field<index>().index : index);

  clsInfo.each_subclass([](auto info, int) {
    using S = decltype(info)::class_t;
    string_view parent = "<none>";
    if constexpr (has_base_class<S>()) {
      parent = class_info<TrefBaseClass(S)>().name;
    }

    printf("==================\n");
    printf("type: %6s, parent: %6s, size: %d\n", info.name.data(),
           parent.data(), info.size);
    printf("--- members ---\n");

    int preLv = 0;
    class_info<S>().each_field([&](auto info, int lv) {
      if (lv != preLv) {
        auto owner = class_info<decltype(info)::enclosing_class_t>().name;
        printf("--- from %s ---\n", owner.data());
      }
      preLv = lv;

      printf("%-2d:%-12s: type: %s", info.index, info.name.data(),
             typeid(info.value).name());

      if constexpr (std::is_base_of_v<Meta, decltype(info.meta)>) {
        printf(", meta: %s\n", info.meta.to_string().c_str());
      } else {
        printf("\n");
      }
      return true;
    });

    return true;
  });
}

struct Meta {
  const char* desc;

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc;
    return o.str();
  }
};

template <typename T>
struct MetaNumber : Meta {
  T minV, maxV;

  constexpr MetaNumber(const char* desc_, T minV_, T maxV_)
      : Meta{desc_}, minV(minV_), maxV(maxV_) {}

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc << ",range:[" << minV << "," << maxV << "]";
    return o.str();
  }
};

struct MetaExportedClass {
  template <typename T>
  static void dumpAll() {
    printf("===== All Exported Class ====\n");
    class_info<T>().each_subclass([&](auto info, int) {
      if constexpr (is_base_of_v<MetaExportedClass, decltype(info.meta)>) {
        printf("%s\n", info.name.data());
      }
      return true;
    });
    puts("============");
  }
};

template <typename T, typename... Args>
struct MetaValidatableFunc {
  using Validate = bool (*)(T&, Args...);
  Validate validate = nullptr;

  constexpr MetaValidatableFunc(Validate vv) : validate(vv) {}
  std::string to_string() { return ""; };
};

struct MetaHookableFunc {};

/////////////////////////////////////

struct Base {
  TrefRootType(Base);

  int baseVal;
  TrefField(baseVal);
};

template <typename T>
struct Data : Base {
  TrefSubType(Data);

  T t;
  TrefFieldWithMeta(t, Meta{"test"});

  int x, y;
  TrefFieldWithMeta(x, (MetaNumber{"pos x", 1, 100}));
  TrefFieldWithMeta(y, (MetaNumber{"pos y", 1, 100}));

  std::string name{"boo"};
  TrefFieldWithMeta(name, Meta{"entity name"});
};

struct Child : Data<int> {
  TrefSubType(Child);

  float z;
  TrefField(z);
};

struct Child2 : Data<float> {
  TrefSubType(Child2);

  float zz;
  TrefField(zz);
};

struct SubChild : Child2 {
  TrefSubTypeWithMeta(SubChild, MetaExportedClass{});

  int subVal = 99;

  const char* ff = "subchild";
  TrefField(ff);

  void func(int a) {
    printf("func called with arg:%d, subVal:%d\n", a, subVal);
  }
  static bool func_validate(self_t& self, int a) {
    printf("check if arg > 0: arg=%d", a);
    return a > 0;
  }
  TrefFieldWithMeta(func, MetaValidatableFunc{func_validate});

  function<void(self_t&, int)> hookableFunc = [](self_t& self, int a) {
    printf("hookable func called with arg: %d, str:%s, subVal:%d\n", a, self.ff,
           self.subVal);
  };
  TrefFieldWithMeta(hookableFunc, MetaHookableFunc{});
};

void TestHookable() {
  printf("======== Test Hookable =========\n");
  SubChild s;

  // call validatable functions
  auto callValidatableFunc = [&](int arg) {
    class_info<SubChild>().each_field([&](auto info, int) {
      using ValidateFunc = MetaValidatableFunc<SubChild, int>;
      if constexpr (std::is_base_of_v<ValidateFunc, decltype(info.meta)>) {
        auto v = info.value;
        if (info.meta.validate(s, arg)) {
          printf("\t validation passed\n");
          (s.*v)(arg);
        } else {
          printf("\t validation failed\n");
        }
      }
      return true;
    });
  };

  callValidatableFunc(-1);
  callValidatableFunc(100);

  // call hookable functions
  class_info<SubChild>().each_field([&](auto info, int) {
    if constexpr (std::is_base_of_v<MetaHookableFunc, decltype(info.meta)>) {
      auto v = info.value;
      auto f = s.*v;
      s.*v = [ff = move(f)](SubChild& self, int a) {
        printf("before hook:%d\n", a);
        ff(self, a);
      };
    }
    return true;
  });
  s.hookableFunc(s, 10);
  printf("====================\n");
}

template <typename T>
struct TempSubChild : SubChild {
  TrefSubType(TempSubChild);

  T newVal;
  TrefField(newVal);
};

struct SubChildOfTempSubChild1 : TempSubChild<int> {
  TrefSubTypeWithMeta(SubChildOfTempSubChild1, MetaExportedClass{});
};

struct SubChildOfTempSubChild2 : TempSubChild<float> {
  TrefSubTypeWithMeta(SubChildOfTempSubChild2, MetaExportedClass{});
};

struct SubChildOfTempSubChild3 : TempSubChild<double> {
  TrefSubTypeWithMeta(SubChildOfTempSubChild3, MetaExportedClass{});
};

//////////////////////////////////////////////////////////////////////////
// reflection for external type

struct ExternalData : SubChild {
  int age;
  int age2;
  float money;
};

struct BindExternalData {
  TrefExternalSubTypeWithMeta(ExternalData, SubChild, (FakeMeta{333, 444}));
  
  TrefField(age);
  TrefField(age2);
  TrefField(money);
};

static_assert(tref::is_reflected<ExternalData>());
static_assert(tref::has_base_class<ExternalData>());
static_assert(is_same_v<TrefBaseClass(ExternalData), SubChild>);
static_assert(class_info<ExternalData>().meta.foo == 333);
static_assert(class_info<ExternalData>().meta.bar == 444);

//////////////////////////////////////////////////////////////////////////
// subclass system test

// Test the subclass system behind all the definition of subclasses,
// Please see the `NOTE` of the each_subclass_r function.
static_assert(hasSubclass<Base>("SubChild"));
static_assert(hasSubclass<Base>("TempSubChild"));
static_assert(hasSubclass<Base>("SubChildOfTempSubChild1"));
static_assert(hasSubclass<Base>("SubChildOfTempSubChild2"));
static_assert(hasSubclass<Base>("SubChildOfTempSubChild3"));
static_assert(hasSubclass<SubChild>("ExternalData"));
static_assert(hasSubclass<Base>("ExternalData"));

void TestReflection() {
  TestEnum();
  dumpTree<Base>();
  dumpDetails<Child2>();
  MetaExportedClass::dumpAll<Base>();
  TestHookable();
}
