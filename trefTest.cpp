#include <functional>
#include <iostream>
#include <sstream>

#include "tref.h"

using namespace std;

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

// using namespace tref;

struct Base {
  RefTypeRoot(Base);
};
static_assert(tref::is_reflected_v<Base>);

template <typename T>
struct Data : Base {
  RefType(Data);

  T t;
  RefMemberWithMeta(t, Meta{"test"});

  int x, y;
  RefMemberWithMeta(x, NumberMeta{"pos x", 1, 100});
  RefMemberWithMeta(y, NumberMeta{"pos y", 1, 100});

  std::string name{"boo"};
  RefMemberWithMeta(name, Meta{"entity name"});
};

struct Child : Data<int> {
  RefType(Child);

  float z;
  RefMember(z);
};

struct Child2 : Data<float> {
  RefType(Child2);

  float zz;
  RefMember(zz);
};

struct SubChild : Child2 {
  RefType(SubChild);

  const char* ff = "subchild";
  RefMember(ff);

  void func(int a) { printf("func called with arg:%d\n", a); }
  static bool func_validate(self& thiz, int a) {
    printf("do validation with arg:%d", a);
    return a > 0;
  }
  RefMemberWithMeta(func, ValidatableFuncMeta{func_validate});

  function<void(self&, int)> hookableFunc = [](self& thiz, int a) {
    printf("hookable func called with arg: %s, %d\n", thiz.ff, a);
  };
  RefMemberWithMeta(hookableFunc, HookableFuncMeta{});
};

struct ExternalData : SubChild {
  int age;
  int age2;
  float money;
};

struct BindExternalData {
  _RefTypeCommon(ExternalData);
  _RefSuper(SubChild);
  RefMember(age);
  RefMember(age2);
  RefMember(money);
};
static_assert(tref::has_super_v<ExternalData>);
static_assert(is_same_v<tref::super_class_t<ExternalData>, SubChild>);

template <typename T>
constexpr bool hasSubClass(const string_view& name) {
  using namespace tref;
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
  using namespace tref;
  printf("===== All Subclass of %s====\n", get<0>(class_meta_v<T>));
  eachSubclass<T>([&](auto* c, auto info, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");
    printf("%s\n", get<0>(info));
    return true;
  });
  puts("============");
}

void TestHookable() {
  using namespace tref;
  SubChild s;

  // call validatable functions
  eachMember<SubChild>([&](auto name, auto v, auto meta, int level) {
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
  eachMember<SubChild>([&](auto name, auto v, auto meta, int level) {
    if constexpr (std::is_base_of_v<HookableFuncMeta, decltype(meta)>) {
      auto f = s.*v;
      s.*v = [ff = move(f)](SubChild& thiz, int a) {
        printf("before hook:%d\n", a);
        ff(thiz, a);
      };
    }
    return true;
  });
  s.hookableFunc(s, 10);
}

void dumpDetails() {
  using namespace tref;

  puts("==== subclass details Base ====");
  eachSubclass<Child2>([](auto c, auto info, int level) {
    using T = remove_pointer_t<decltype(c)>;
    auto [clsName, sz] = info;
    auto parent = "";
    if constexpr (has_super_v<T>) {
      parent = get<0>(class_meta_v<super_class_t<T>>);
    }

    printf("=====\n");
    printf("type:%s, parent:%s, sz: %d\n", clsName, parent, sz);

    int cnt = 1;
    eachMember<T>([&](auto name, auto ptr, auto meta, int level) {
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

  Child d[2];
  JsonReader r(R"(
[{  x 1 y 2 z 3 } { x 11 y 22 z 33}]
)");
  r >> d;

  puts("==== field values of Child ====");
  int cnt = 1;
  auto& o = d[0];
  eachMember<Child>([&](auto name, auto v, auto meta, int level) {
    if constexpr (std::is_base_of_v<Meta, decltype(meta)>) {
      printf("field %d:%s, %s,", cnt, name, meta.to_string().c_str());
    } else {
      printf("field %d:%s,", cnt, name);
    }
    cout << "value:" << o.*v << endl;
    cnt++;
    return true;
  });
}

void TestRef() {
  dumpTree<Base>();
  dumpDetails();
  TestHookable();
}
