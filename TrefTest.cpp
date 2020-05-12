#include <functional>
#include <iostream>
#include <sstream>

#include "tref.h"

using namespace std;
using namespace tref;

//////////////////////////////////////////////////////////////////////////
// basic usage

//////////////////////////////
// simple class

struct TypeA {
  TrefType(TypeA);

  int val;
  TrefMember(val);
};

static_assert(is_reflected<TypeA>());
static_assert(class_info<TypeA>().name == "TypeA");
static_assert(class_info<TypeA>().size == sizeof(TypeA));
static_assert(class_info<TypeA>().each_direct_member([](auto info) {
  using mem_t = decltype(info.value);
  return info.name == "val" && is_same_v<enclosing_class_t<mem_t>, TypeA> &&
         is_same_v<member_t<mem_t>, decltype(TypeA{}.val)>;
}));
static_assert(class_info<TypeA>().each_member_r([](auto info, int lv) {
  return lv == 0 && info.name == "val";
}));
static_assert(class_info<TypeA>().get_member_index("val") == 1);
static_assert(class_info<TypeA>().get_member<1>().index == 1);
static_assert(class_info<TypeA>().get_member<1>().name == "val");
static_assert(
    is_same_v<decltype(class_info<TypeA>().get_member<1>())::member_t, int>);

//////////////////////////////
// subclass

struct TypeB : TypeA {
  TrefType(TypeB);
  float foo;
  TrefMember(foo);
};

static_assert(has_base_class<TypeB>());
static_assert(is_same_v<TrefBaseClass(TypeB), TypeA>);
static_assert(is_same_v<decltype(class_info<TypeB>())::base_t, TypeA>);
static_assert(class_info<TypeB>().each_direct_member([](auto info) {
  using mem_t = decltype(info.value);
  return info.name == "foo" && is_same_v<enclosing_class_t<mem_t>, TypeB> &&
         is_same_v<member_t<mem_t>, decltype(TypeB{}.foo)>;
}));
static_assert(class_info<TypeB>().each_member_r([](auto info, int lv) {
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
  TrefMember(tempVal);
};

static_assert(class_info<TempType<int>>().name == "TempType");
static_assert(class_info<TempType<int>>().each_direct_member([](auto info) {
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
  TrefMember(a);
};

struct UsingTemplateIntance {
  TrefType(UsingTemplateIntance);

  ReflectedTemplate<int, float> a;
  TrefMember(a);
};

static_assert(class_info<decltype(UsingTemplateIntance{}.a)>().name ==
              "ReflectedTemplate");
static_assert(
    class_info<UsingTemplateIntance>().each_direct_member([](auto info) {
      using MT = decltype(info)::member_t;
      return class_info<MT>().name == "ReflectedTemplate";
    }));

//////////////////////////////

struct TestIsMember {
  TrefType(TestIsMember);

  static constexpr auto staticVal = 0;
  TrefMember(staticVal);

  int memberVal;
  TrefMember(memberVal);
};

static_assert(class_info<TestIsMember>().each_direct_member([](auto info) {
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

  TrefMemberWithMeta((MultiArgsVar_v<int, int>),
                     (MetaMultiArgs<float, char>{}));

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
    TrefMember(a);
  };

  TrefMemberType(InnerTemplate<int>);
};

static_assert(class_info<TestInnerTemplate::InnerTemplate<int>>().name ==
              "InnerTemplate");

static_assert(class_info<TestInnerTemplate::InnerTemplate<int>>()
                  .each_direct_member([](auto info) {
                    static_assert(is_same_v<decltype(info)::member_t, int>);
                    return info.name == "a";
                  }));

static_assert(
    class_info<TestInnerTemplate>().each_direct_member_type([](auto info) {
      using MT = decltype(info.value)::type;

      static_assert(is_same_v<MT, TestInnerTemplate::InnerTemplate<int>>);
      static_assert(class_info<MT>().name == "InnerTemplate");

      return info.name == "InnerTemplate<int>";
    }));

//////////////////////////////////////////////////////////////////////////
// enum test

// global enum

struct FakeEnumMeta {
  int foo;
  int bar;
};

TrefEnumGlobalWithMeta(EnumA,
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
static_assert(enum_info<EnumA>().each_item([](auto name, auto val) {
  switch (val) {
    case EnumA::Ass:
      return name == "Ass";
    case EnumA::Ban:
      return name == "Ban";
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

// enum in class

struct DataWithEnumMemType {
  TrefType(DataWithEnumMemType);

  TrefEnum(EnumF, ValA = 1, ValB = 12);
  TrefMemberType(EnumF);
};
static_assert(
    class_info<DataWithEnumMemType>().each_direct_member_type([](auto info) {
      using T = decltype(info.value)::type;
      static_assert(is_enum_v<T> && is_same_v<T, DataWithEnumMemType::EnumF>);
      return info.name == "EnumF";
    }));

template <typename T>
void DumpEnum() {
  printf("========= Enum Members of %s ======\n", enum_info<T>().name.data());
  enum_info<T>().each_item([](auto name, auto val) {
    printf("name: %.*s, val: %d\n", name.size(), name.data(), (int)val);
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
  class_info<T>().each_subclass_r([&](auto info, int) {
    found = info.name == name;
    return !found;
  });
  return found;
}

template <class T>
void dumpTree() {
  printf("===== All Subclass of %s ====\n", class_info<T>().name.data());

  class_info<T>().each_subclass_r([&](auto info, int level) {
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
  constexpr auto index = clsInfo.get_member_index(memName);
  printf("index of %s: %d\n", memName,
         index > 0 ? clsInfo.get_member<index>().index : index);

  clsInfo.each_subclass_r([](auto info, int) {
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
    class_info<S>().each_member_r([&](auto info, int lv) {
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
    class_info<T>().each_subclass_r([&](auto info, int) {
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
  TrefMember(baseVal);
};

template <typename T>
struct Data : Base {
  TrefSubType(Data);

  T t;
  TrefMemberWithMeta(t, Meta{"test"});

  int x, y;
  TrefMemberWithMeta(x, (MetaNumber{"pos x", 1, 100}));
  TrefMemberWithMeta(y, (MetaNumber{"pos y", 1, 100}));

  std::string name{"boo"};
  TrefMemberWithMeta(name, Meta{"entity name"});
};

struct Child : Data<int> {
  TrefSubType(Child);

  float z;
  TrefMember(z);
};

struct Child2 : Data<float> {
  TrefSubType(Child2);

  float zz;
  TrefMember(zz);
};

struct SubChild : Child2 {
  TrefSubTypeWithMeta(SubChild, MetaExportedClass{});

  int subVal = 99;

  const char* ff = "subchild";
  TrefMember(ff);

  void func(int a) {
    printf("func called with arg:%d, subVal:%d\n", a, subVal);
  }
  static bool func_validate(self_t& self, int a) {
    printf("check if arg > 0: arg=%d", a);
    return a > 0;
  }
  TrefMemberWithMeta(func, MetaValidatableFunc{func_validate});

  function<void(self_t&, int)> hookableFunc = [](self_t& self, int a) {
    printf("hookable func called with arg: %d, str:%s, subVal:%d\n", a, self.ff,
           self.subVal);
  };
  TrefMemberWithMeta(hookableFunc, MetaHookableFunc{});
};

void TestHookable() {
  printf("======== Test Hookable =========\n");
  SubChild s;

  // call validatable functions
  auto callValidatableFunc = [&](int arg) {
    class_info<SubChild>().each_member_r([&](auto info, int) {
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
  class_info<SubChild>().each_member_r([&](auto info, int) {
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
  TrefMember(newVal);
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
  TrefMember(age);
  TrefMember(age2);
  TrefMember(money);
};

static_assert(tref::is_reflected<ExternalData>());
// static_assert(tref::has_base_class<ExternalData>());
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
