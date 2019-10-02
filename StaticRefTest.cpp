#include "pch.h"

#include <functional>
#include <iostream>
#include <sstream>

using namespace std;

struct Meta {
  const char *desc;

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc;
    return o.str();
  }
};

template <typename T> struct NumberMeta : Meta {
  T minV, maxV;

  constexpr NumberMeta(const char *desc_, T minV_, T maxV_)
      : Meta{desc_}, minV(minV_), maxV(maxV_) {}

  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc << ",range:[" << minV << "," << maxV << "]";
    return o.str();
  }
};

template <typename T, typename... Args> struct ValidatableFuncMeta {
  using Validate = bool (*)(T &, Args...);
  Validate validate = nullptr;

  constexpr ValidatableFuncMeta(Validate vv) : validate(vv) {}
  std::string to_string() { return ""; };
};

struct HookableFuncMeta {
  constexpr HookableFuncMeta() {}
};

// using namespace tref;

struct Base {
  ReflectedTypeRoot(Base);
};

template <typename T> struct Data : Base {
  ReflectedType(Data);

  T t;
  ReflectedMeta(t, Meta{"test"});

  int x, y;
  ReflectedMeta(x, NumberMeta{"pos x", 1, 100});
  ReflectedMeta(y, NumberMeta{"pos y", 1, 100});

  std::string name{"boo"};
  ReflectedMeta(name, Meta{"entity name"});
};

static_assert(tref::IsReflected<Data<int>>::value);

struct Child : Data<int> {
  ReflectedType(Child);

  float z;
  Reflected(z);
};

struct Child2 : Data<int> {
  ReflectedType(Child2);

  float zz;
  Reflected(zz);
};

struct SubChild : Child2 {
  ReflectedType(SubChild);

  const char *ff = "subchild";
  Reflected(ff);

  void func(int a) { printf("func called with arg:%d\n", a); }
  static bool func_validate(self &thiz, int a) {
    printf("do validation with arg:%d", a);
    return a > 0;
  }
  ReflectedMeta(func, ValidatableFuncMeta{func_validate});

  function<void(self &, int)> hookableFunc = [](self &thiz, int a) {
    printf("hookable func called with arg: %s, %d\n", thiz.ff, a);
  };
  ReflectedMeta(hookableFunc, HookableFuncMeta{});
};

static_assert(string_view("test").length() == 4);

constexpr bool hasClass(const string_view &name) {
  using namespace tref;
  return ([&](auto *cls, int) { return name == get<0>(cls->__meta); })(
      (SubChild *)0, 0);
}

static_assert(hasClass("SubChild2"));

template <class T> void dumpTree() {
  using namespace tref;
  printf("===== All Subclass of %s====\n", get<0>(T::__meta));
  eachSubClass<T>([&](auto *c, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");
    printf("%s\n", get<0>(c->__meta));
    return true;
  });
  puts("============");
}

void TestHookable() {
  using namespace tref;
  SubChild s;

  // call validatable functions
  eachFields<SubChild>([&](auto name, auto v, auto meta, int level) {
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
  eachFields<SubChild>([&](auto name, auto v, auto meta, int level) {
    if constexpr (std::is_base_of_v<HookableFuncMeta, decltype(meta)>) {
      auto f = s.*v;
      s.*v = [ff = move(f)](SubChild &thiz, int a) {
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

  puts("==== subclass details Data ====");
  eachSubClass<Data<int>>([](auto c, int level) {
    auto [clsName, file, line] = c->__meta;
    printf("type:%s, file:%s(%d)\n", clsName, file, line);
    using T = remove_pointer_t<decltype(c)>;

    int cnt = 1;
    eachFields<T>([&](auto name, auto v, auto meta, int level) {
      if constexpr (std::is_base_of_v<Meta, decltype(meta)>) {
        printf("field %d:%s, type:%s, %s\n", cnt, name, typeid(v).name(),
               meta.to_string().c_str());
      } else {
        printf("field %d:%s, type:%s\n", cnt, name, typeid(v).name());
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
  auto &o = d[0];
  eachFields<Child>([&](auto name, auto v, auto meta, int level) {
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
  dumpTree<Data<int>>();
  dumpDetails();
  TestHookable();
}