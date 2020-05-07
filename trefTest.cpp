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
constexpr auto info = class_meta_v<Base>;

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
    auto [clsName, sz, basePtr] = info;
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

TrefEnumGlobal(EnumA, Ass = 1, Ban = 4);
static_assert(enum_to_string(EnumA::Ass) == "Ass");
static_assert(string_to_enum("Ban", EnumA::Ass) == EnumA::Ban);

void TestEnum() {
  printf("========= Enum Members of %s ======\n", "EnumA");
  enum_each<EnumA>([](auto val, auto name) {
    printf("name: %.*s, val: %d\n", name.size(), name.data(), (int)val);
  });
  puts("==================");
}

void TestRef() {
  TestEnum();
  dumpTree<Base>();
  dumpDetails();
  TestHookable();
}
