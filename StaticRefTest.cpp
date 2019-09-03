#include "pch.h"
#include <iostream>
#include <sstream>

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
  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc << ",range:[" << minV << "," << maxV << "]";
    return o.str();
  }
};

// using namespace tref;

template <typename T> struct Data {
  T t;

  int x, y;
  std::string name{"boo"};
  RefRoot(Data, RefFieldMeta(x, NumberMeta<int>{"pos x", 1, 100}),
          RefFieldMeta(y, NumberMeta<int>{"pos y", 1, 100}),
          RefFieldMeta(name, Meta{"entity name"}),
          RefFieldMeta(t, Meta{"test"}));
};

struct Child : Data<int> {
  float z;
  RefType(Child, RefField(z));
};

struct Child2 : Data<int> {
  float zz;
  RefType(Child2, RefField(zz));
};

struct SubChild : Child2 {
  const char *ff = "subchild";
  RefType(SubChild, RefField(ff));
};

template <class T> void dumpTree() {
  using namespace tref;
  printf("===== All Subclass of %s====\n", T::__name);
  eachSubClass<T>([&](auto *c, int level) {
    for (int i = 0; i < 4 * level; i++)
      printf(" ");
    printf("%s\n", c->__name);
    return true;
  });
  puts("============");
}

void TestRef() {
  using namespace tref;
  dumpTree<Data<int>>();

  puts("==== subclass details Data ====");
  auto tp = tref::subclassOf<Data<int>>();
  tuple_for(tp, [](auto c) {
    puts(c->__name);
    using T = remove_pointer_t<decltype(c)>;

    int cnt = 1;
    tuple_for(fieldsOf<T>(), [&](auto &p) {
      auto [name, v, meta] = p;
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
  tuple_for(fieldsOf<Child>(), [&](auto &p) {
    auto [name, v, meta] = p;
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