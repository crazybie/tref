#include "tref.h"

#include <functional>
#include <iostream>
#include <sstream>

using namespace std;
using namespace tref;

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

struct Base {
  TrefTypeRoot(Base);
};
static_assert(tref::is_reflected_v<Base>);

template <typename T>
struct Data : Base {
  TrefType(Data);

  T t;
  TrefMemberWithMeta(t, Meta{"test"});

  int x, y;
  TrefMemberWithMeta(x, NumberMeta{"pos x", 1, 100});
  TrefMemberWithMeta(y, NumberMeta{"pos y", 1, 100});

  std::string name{"boo"};
  TrefMemberWithMeta(name, Meta{"entity name"});
};

struct Child : Data<int> {
  TrefType(Child);

  float z;
  TrefMember(z);
};

struct Child2 : Data<float> {
  TrefType(Child2);

  float zz;
  TrefMember(zz);
};

struct SubChild : Child2 {
  TrefType(SubChild);

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

struct ExternalData : SubChild {
  int age;
  int age2;
  float money;
};

struct BindExternalData {
  TrefTypeExternal(ExternalData, SubChild);
  TrefMember(age);
  TrefMember(age2);
  TrefMember(money);
};
static_assert(tref::has_base_v<ExternalData>);
static_assert(is_same_v<tref::base_class_t<ExternalData>, SubChild>);
static_assert(tref::is_reflected_v<ExternalData>);

template <typename T>
constexpr bool hasSubClass(const string_view& name) {
  auto found = false;
  imp::each<T, imp::SubclassTag>([&](auto info) {
    using C = remove_pointer_t<tuple_element_t<1, decltype(info)>>;
    if (name == get<0>(class_meta_v<C>)) {
      found = true;
      return false;
    }
    if (hasSubClass<C>(name)) {
      found = true;
      return false;
    }
    return true;
  });
  return found;
}

static_assert(hasSubClass<Base>("SubChild"));
static_assert(hasSubClass<Base>("ExternalData"));

template <class T>
void dumpTree() {
  printf("===== All Subclass of %s====\n", get<0>(class_meta_v<T>));

  each_subclass<T>([&](auto* c, auto info, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");
    printf("%s\n", get<0>(info));
    return true;
  });
  puts("============");
}

void TestHookable() {
  SubChild s;

  // call validatable functions
  each_member<SubChild>([&](auto name, auto v, auto meta, int level) {
    using ValidateFunc = ValidatableFuncMeta<SubChild, int>;
    if constexpr (std::is_base_of_v<ValidateFunc, decltype(meta)>) {
      auto arg = -1;
      if (meta.validate(s, arg)) {
        (s.*v)(arg);
      }
    }
    return true;
  });

  // call hookable functions
  each_member<SubChild>([&](auto name, auto v, auto meta, int level) {
    if constexpr (std::is_base_of_v<HookableFuncMeta, decltype(meta)>) {
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

void dumpDetails() {
  using T = Child2;
  printf("==== subclass details of %s ====\n", get<0>(class_meta_v<T>));

  each_subclass<T>([](auto c, auto info, int level) {
    using T = remove_pointer_t<decltype(c)>;
    auto [clsName, sz] = info;
    auto parent = "";
    if constexpr (has_base_v<T>) {
      parent = get<0>(class_meta_v<base_class_t<T>>);
    }

    printf("=====\n");
    printf("type:%s, parent:%s, sz: %d\n", clsName, parent, sz);

    int cnt = 1;
    each_member<T>([&](auto name, auto ptr, auto meta, int level) {
      if constexpr (std::is_base_of_v<Meta, decltype(meta)>) {
        printf("field %d:%s, type:%s, %s\n", cnt, name, typeid(ptr).name(),
               meta.to_string().c_str());
      } else {
        printf("field %d:%s, type:%s\n", cnt, name, typeid(ptr).name());
      }
      cnt++;
      return true;
    });

    return true;
  });
}

//////////////////////////////////////////////////////////////////////////
/// enum reflection

namespace tref_enum {
namespace imp {

#define MAP(macro, ...) \
  IDENTITY(APPLY(CHOOSE_MAP_START, COUNT(__VA_ARGS__))(macro, __VA_ARGS__))

#define CHOOSE_MAP_START(count) MAP##count

#define APPLY(macro, ...) IDENTITY(macro(__VA_ARGS__))

// Needed to expand __VA_ARGS__ "eagerly" on the MSVC preprocessor.
#define IDENTITY(x) x

#define MAP1(m, x) m(x)
#define MAP2(m, x, ...) m(x) IDENTITY(MAP1(m, __VA_ARGS__))
#define MAP3(m, x, ...) m(x) IDENTITY(MAP2(m, __VA_ARGS__))
#define MAP4(m, x, ...) m(x) IDENTITY(MAP3(m, __VA_ARGS__))
#define MAP5(m, x, ...) m(x) IDENTITY(MAP4(m, __VA_ARGS__))
#define MAP6(m, x, ...) m(x) IDENTITY(MAP5(m, __VA_ARGS__))
#define MAP7(m, x, ...) m(x) IDENTITY(MAP6(m, __VA_ARGS__))
#define MAP8(m, x, ...) m(x) IDENTITY(MAP7(m, __VA_ARGS__))

#define EVALUATE_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, count, ...) count

#define COUNT(...) IDENTITY(EVALUATE_COUNT(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1))

#define STRINGIZE_SINGLE(E)                    \
  std::tuple{(tref_enum::imp::ignore_assign)E, \
             tref_enum::imp::_tref_trim_value(#E)},

#define STRINGIZE(...) IDENTITY(MAP(STRINGIZE_SINGLE, __VA_ARGS__))

struct ignore_assign {
  constexpr ignore_assign(int value) : _value(value) {}
  constexpr operator int() const { return _value; }
  constexpr ignore_assign operator=(int) { return *this; }
  int _value;
};

#define _Enum(T, ...)                                    \
  enum T { __VA_ARGS__ };                                \
  constexpr auto _tref_enum_items(T) {                   \
    return std::tuple{IDENTITY(STRINGIZE(__VA_ARGS__))}; \
  }

constexpr auto tuple_for = [](auto&& t, auto&& f) {
  return apply([&f](auto&... x) { (..., f(x)); }, t);
};

constexpr string_view _tref_trim_value(string_view s) {
  auto p = s.find_first_of(' ');
  if (p == string_view::npos)
    p = s.find_first_of('=');
  return s.substr(0, p);
}

template <typename T>
constexpr auto enum_string(T v) {
  constexpr auto items = _tref_enum_items((T)0);
  string_view ret = "";
  tuple_for(items, [&](auto e) {
    if ((int)get<0>(e) == v) {
      ret = get<1>(e);
    }
  });
  return ret;
}

template <typename T>
constexpr auto enum_value(string_view s, T defVal) {
  constexpr auto items = _tref_enum_items((T)0);
  auto ret = defVal;
  tuple_for(items, [&](auto n) {
    if (s == get<1>(n)) {
      ret = (T)(int)get<0>(n);
    }
  });
  return ret;
}
}  // namespace imp
using imp::enum_string;
using imp::enum_value;

}  // namespace tref_enum

_Enum(EnumA, Ass = 1, Ban = 4);
static_assert(tref_enum::enum_string(Ass) == "Ass");
static_assert(tref_enum::enum_value("Ban", Ass) == Ban);

void TestRef() {
  auto f = tref_enum::enum_string(Ass);
  dumpTree<Base>();
  dumpDetails();
  TestHookable();
}
