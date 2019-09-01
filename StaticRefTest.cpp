#include "pch.h"
#include <iostream>
#include <sstream>

template <typename T> struct Meta {
  char *desc;
  T minV, maxV;
  std::string to_string() {
    std::ostringstream o;
    o << "desc:" << desc << ",range:[" << minV << "," << maxV << "]";
    return o.str();
  }
};

// using namespace tref;

struct Data {
  int x, y;
  std::string name{"boo"};
  RefRoot(Data, RefFieldMeta(x, Meta<int>{"pos x", 1, 100}),
          RefFieldMeta(y, Meta<int>{"pos y", 1, 100}),
          RefFieldMeta(name, Meta<int>{"entity name", 1, 100}));
};

struct Child : Data {
  float z;
  RefType(Child, RefField(z));
};

struct Child2 : Data {
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
  dumpTree<Data>();

  puts("==== subclass details Data ====");
  auto tp = tref::subclassOf<Data>();
  tuple_for(tp, [](auto c) {
    puts(c->__name);
    using T = std::remove_reference_t<decltype(*c)>;

    int cnt = 1;
    tuple_for(fieldsOf<T>(), [&](auto &p) {
      auto [name, v, meta] = p;
      if constexpr (std::is_same_v<decltype(meta), Meta<int>>) {
        printf("field %d:%s, %s\n", cnt, name, meta.to_string().c_str());
      } else {
        printf("field %d:%s\n", cnt, name);
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
    if constexpr (std::is_same_v<decltype(meta), Meta<int>>) {
      printf("field %d:%s, %s,", cnt, name, meta.to_string().c_str());
    } else {
      printf("field %d:%s,", cnt, name);
    }
    cout << "value:" << o.*v << endl;
    cnt++;
    return true;
  });
}