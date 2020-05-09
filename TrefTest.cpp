#include <functional>
#include <iostream>
#include <sstream>

#include "tref.h"

using namespace std;
using namespace tref;

//////////////////////////////////////////////////////////////////////////
// basic usage

struct TypeA {
  TrefType(TypeA);
  int val;
  TrefMember(val);
};

static_assert(is_reflected_v<TypeA>);
static_assert(class_info_v<TypeA>.name == "TypeA");
static_assert(class_info_v<TypeA>.size == sizeof(TypeA));
static_assert(class_info_v<TypeA>.each_direct_member([](auto info) {
  using mem_t = decltype(info.value);
  return info.name == "val" && is_same_v<object_t<mem_t>, TypeA> &&
         is_same_v<remove_object_t<mem_t>, decltype(TypeA{}.val)>;
}));
static_assert(class_info_v<TypeA>.each_member_r([](auto info, int lv) {
  return lv == 0 && info.name == "val";
}));

struct TypeB : TypeA {
  TrefType(TypeB);
  float foo;
  TrefMember(foo);
};

static_assert(is_same_v<base_class_t<TypeB>, TypeA>);
static_assert(is_same_v<decltype(class_info_v<TypeB>.base)::type, TypeA>);
static_assert(class_info_v<TypeB>.each_direct_member([](auto info) {
  using mem_t = decltype(info.value);
  return info.name == "foo" && is_same_v<object_t<mem_t>, TypeB> &&
         is_same_v<remove_object_t<mem_t>, decltype(TypeB{}.foo)>;
}));
static_assert(class_info_v<TypeB>.each_member_r([](auto info, int lv) {
  return (lv == 0 || lv == 1) && (info.name == "foo" || info.name == "val");
}));

template <typename T>
struct TempType : TypeB {
  TrefType(TempType);
  T tempVal;
  TrefMember(tempVal);
};

static_assert(class_info_v<TempType<int>>.name == "TempType");
static_assert(class_info_v<TempType<int>>.each_direct_member([](auto info) {
  using mem_t = decltype(info.value);
  return info.name == "tempVal" && is_same_v<object_t<mem_t>, TempType<int>> &&
         is_same_v<remove_object_t<mem_t>, int>;
}));

struct SubTypeA : TempType<int> {
  TrefType(SubTypeA);
};
static_assert(is_same_v<base_class_t<SubTypeA>, TempType<int>>);
static_assert(!is_same_v<base_class_t<SubTypeA>, TempType<float>>);

struct SubTypeB : TempType<float> {
  TrefType(SubTypeB);
};

static_assert(is_same_v<base_class_t<SubTypeB>, TempType<float>>);
static_assert(!is_same_v<base_class_t<SubTypeB>, TempType<int>>);

//////////////////////////////////////////////////////////////////////////
// enum test

TrefEnumGlobal(EnumA, Ass = 1, Ban = (int)EnumA::Ass * 3);
static_assert(enum_to_string(EnumA::Ass) == "Ass");
static_assert(string_to_enum("Ban", EnumA::Ass) == EnumA::Ban);

enum class ExternalEnum { Value1 = 1, Value2 = Value1 + 4 };
TrefEnumImp(ExternalEnum, Value1, Value2);
static_assert(enum_info_v<ExternalEnum>.name == "ExternalEnum");
static_assert(enum_info_v<ExternalEnum>.size == sizeof(ExternalEnum));
static_assert(enum_info_v<ExternalEnum>.items.size() == 2);

template <typename T>
void DumpEnum() {
  printf("========= Enum Members of %s ======\n", enum_info_v<T>.name.data());
  enum_info_v<T>.each_item([](auto name, auto val) {
    printf("name: %.*s, val: %d\n", name.size(), name.data(), (int)val);
  });
  puts("==================");
}

void TestEnum() {
  DumpEnum<EnumA>();
  DumpEnum<ExternalEnum>();
}

struct DataWithEnumMemType {
  TrefType(DataWithEnumMemType);
  TrefEnum(EnumF, ValA = 1, ValB = 12);
  TrefMemberType(EnumF);
};
static_assert(
    class_info_v<DataWithEnumMemType>.each_direct_member_type([](auto info) {
      using T = decltype(info.value)::type;
      static_assert(is_enum_v<T> && is_same_v<T, DataWithEnumMemType::EnumF>);
      return info.name == "EnumF";
    }));

//////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr bool hasSubclass(string_view name) {
  auto found = false;
  class_info_v<T>.each_subclass_r([&](auto info, int) {
    found = info.name == name;
    return !found;
  });
  return found;
}

template <class T>
void dumpTree() {
  printf("===== All Subclass of %s ====\n", class_info_v<T>.name.data());

  class_info_v<T>.each_subclass_r([&](auto info, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");
    printf("%s (rtti: %s)\n", info.name.data(),
           typeid(decltype(info)::class_t).name());
    return true;
  });
  puts("============");
}

template <typename T>
void dumpDetails() {
  printf("==== subclass details of %s ====\n", class_info_v<T>.name.data());

  class_info_v<T>.each_subclass_r([](auto info, int) {
    using S = decltype(info)::class_t;
    string_view parent = "<none>";
    if constexpr (has_base_v<S>) {
      parent = class_info_v<base_class_t<S>>.name;
    }

    printf("==================\n");
    printf("type: %6s, parent: %6s, size: %d\n", info.name.data(),
           parent.data(), info.size);
    printf("--- members ---\n");

    int preLv = 0;
    class_info_v<S>.each_member_r([&](auto info, int lv) {
      if (lv != preLv) {
        auto owner = class_info_v<decltype(info)::class_t>.name;
        printf("--- from %s ---\n", owner.data());
      }
      preLv = lv;

      printf("%-12s: type: %s", info.name.data(), typeid(info.value).name());

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

//////////////////////////////////////////////////////////////////////////
// reflection of hierarchy with custom meta

struct Meta {
  const char* desc;

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc;
    return o.str();
  }
};

template <typename T>
struct NumberMeta : Meta {
  T minV, maxV;

  constexpr NumberMeta(const char* desc_, T minV_, T maxV_)
      : Meta{desc_}, minV(minV_), maxV(maxV_) {}

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc << ",range:[" << minV << "," << maxV << "]";
    return o.str();
  }
};

struct Base {
  TrefRootType(Base);
};

template <typename T>
struct Data : Base {
  TrefSubType(Data);

  T t;
  TrefMemberWithMeta(t, Meta{"test"});

  int x, y;
  TrefMemberWithMeta(x, NumberMeta{"pos x", 1, 100});
  TrefMemberWithMeta(y, NumberMeta{"pos y", 1, 100});

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

template <typename T, typename... Args>
struct ValidatableFuncMeta {
  using Validate = bool (*)(T&, Args...);
  Validate validate = nullptr;

  constexpr ValidatableFuncMeta(Validate vv) : validate(vv) {}
  std::string to_string() { return ""; };
};

struct HookableFuncMeta {
  constexpr HookableFuncMeta() {}
};

struct SubChild : Child2 {
  TrefSubType(SubChild);

  const char* ff = "subchild";
  TrefMember(ff);

  void func(int a) { printf("func called with arg:%d\n", a); }
  static bool func_validate(self_t& self, int a) {
    printf("do validation with arg:%d", a);
    return a > 0;
  }
  TrefMemberWithMeta(func, ValidatableFuncMeta{func_validate});

  function<void(self_t&, int)> hookableFunc = [](self_t& self, int a) {
    printf("hookable func called with arg: %s, %d\n", self.ff, a);
  };
  TrefMemberWithMeta(hookableFunc, HookableFuncMeta{});
};

void TestHookable() {
  SubChild s;

  // call validatable functions
  class_info_v<SubChild>.each_member_r([&](auto info, int) {
    using ValidateFunc = ValidatableFuncMeta<SubChild, int>;
    if constexpr (std::is_base_of_v<ValidateFunc, decltype(info.meta)>) {
      auto v = info.value;
      auto arg = -1;
      if (info.meta.validate(s, arg)) {
        (s.*v)(arg);
      }
    }
    return true;
  });

  // call hookable functions
  class_info_v<SubChild>.each_member_r([&](auto info, int) {
    if constexpr (std::is_base_of_v<HookableFuncMeta, decltype(info.meta)>) {
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
}

template <typename T>
struct TempSubChild : SubChild {
  TrefSubType(TempSubChild);
  T newVal;
  TrefMember(newVal);
};

struct SubChildOfTempSubChild1 : TempSubChild<int> {
  TrefSubType(SubChildOfTempSubChild1);
};

struct SubChildOfTempSubChild2 : TempSubChild<float> {
  TrefSubType(SubChildOfTempSubChild2);
};

struct SubChildOfTempSubChild3 : TempSubChild<double> {
  TrefSubType(SubChildOfTempSubChild3);
};

//////////////////////////////////////////////////////////////////////////
// reflection for external type

struct ExternalData : SubChild {
  int age;
  int age2;
  float money;
};

struct BindExternalData {
  TrefExternalSubType(ExternalData, SubChild);
  TrefMember(age);
  TrefMember(age2);
  TrefMember(money);
};

static_assert(tref::is_reflected_v<ExternalData>);
static_assert(tref::has_base_v<ExternalData>);
static_assert(is_same_v<tref::base_class_t<ExternalData>, SubChild>);

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
  TestHookable();
}
