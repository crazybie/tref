#include <tuple>
#include <type_traits>
#include <utility>

// refs: https://woboq.com/blog/verdigris-implementation-tricks.html
namespace tref {

using namespace std;

template <int N> struct id : public id<N - 1> {
  static constexpr int value = N;
  static constexpr id<N - 1> prev() { return {}; }
};

template <typename T> struct tag;

template <> struct id<0> { static constexpr int value = 0; };

template <typename T, typename F> constexpr auto tuple_for(T &&t, F &&f) {
  return apply([&f](auto &... x) { return (f(x) && ...); }, forward<T>(t));
};

template <typename T, typename F> constexpr bool ref_for(F &&f) {
  return tuple_for(T::w_states(), forward<F>(f));
};

template <typename T, class = void_t<>> struct IsReflected : false_type {};

template <typename T>
struct IsReflected<T, void_t<decltype(&T::w_states)>> : true_type {};

#define _RefReturn(R)                                                          \
  ->decltype(R) { return R; }

#define _RefNextId(preState) tref::id<std::tuple_size_v<decltype(preState)> + 1>

//////////////////////////////////////////////////////////////////////////

#define _ref_last_state() w_state(tref::id<255>{})

#define _ref_type_common(T)                                                    \
protected:                                                                     \
  using _parent = T;                                                           \
                                                                               \
private:                                                                       \
  using self = T;                                                              \
  static constexpr auto __name = #T;                                           \
  static constexpr std::tuple<> w_state(tref::id<0>) { return {}; }

#define ref_root(T)                                                            \
  _ref_type_common(T);                                                         \
                                                                               \
public:                                                                        \
  static constexpr auto w_states() { return _ref_last_state(); }

#define ref_type(T)                                                            \
public:                                                                        \
  using super = _parent;                                                       \
  _ref_type_common(T);                                                         \
                                                                               \
public:                                                                        \
  static constexpr auto w_states() {                                           \
    return std::tuple_cat(_ref_last_state(), super::w_states());               \
  }

#define ref_field_meta(S, ...)                                                 \
  static constexpr auto w_state(_RefNextId(_ref_last_state()) n) _RefReturn(   \
      std::tuple_cat(w_state(n.prev()), std::make_tuple(std::make_tuple(       \
                                            #S, &self::S, __VA_ARGS__))))

#define ref_field(S) ref_field_meta(S, nullptr)

//////////////////////////////////////////////////////////////////////////

#define _RefCommon(T)                                                          \
protected:                                                                     \
  using _parent = T;                                                           \
  using self = T;                                                              \
  static constexpr auto __name = #T;

#define RefRoot(T, ...)                                                        \
  _RefCommon(T);                                                               \
  _RefSubClassRoot();                                                          \
                                                                               \
public:                                                                        \
  static constexpr auto w_states() { return std::make_tuple(__VA_ARGS__); }

#define RefType(T, ...)                                                        \
  using super = _parent;                                                       \
  _RefCommon(T);                                                               \
  _RefSubClassRoot();                                                          \
  _RefSubClass();                                                              \
                                                                               \
public:                                                                        \
  static constexpr auto w_states() {                                           \
    return std::tuple_cat(super::w_states(), std::make_tuple(__VA_ARGS__));    \
  }

#define _RefSubClassRoot()                                                     \
  friend constexpr std::tuple<> subclass(tref::tag<self> *, tref::id<0>) {     \
    return {};                                                                 \
  }

#define _RefSubClass()                                                         \
  friend constexpr auto subclass(                                              \
      tref::tag<super> *,                                                      \
      _RefNextId(subclass((tref::tag<super> *)0, tref::id<255>{})) n)          \
      _RefReturn(std::tuple_cat(subclass((tref::tag<super> *)0, n.prev()),     \
                                std::make_tuple((self *)0)))

#define RefFieldMeta(F, ...) std::make_tuple(#F, &self::F, __VA_ARGS__)
#define RefField(F) RefFieldMeta(F, nullptr)

} // namespace tref

#ifdef JSON_READER

template <typename T,
          typename = std::enable_if_t<tref::IsReflected<T>::value, bool>>
bool operator>>(JsonReader &r, T &d) {
  if (!r.expectObjStart())
    return false;

  for (string key; !r.expectObjEnd();) {
    if (!r.expectObjKey(key))
      return false;

    auto loaded = false;
    tref::ref_for<T>([&](auto &f) {
      if (auto [name, e, meta] = f; name == key) {
        if (r >> d.*e) {
          loaded = true;
        } else {
          r.onInvalidValue(name);
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

#endif

#ifdef REF_TEST
////////////////////////////////////////////

#include <iostream>
using namespace tref;

template <typename T> void printMemVar(T &t) {
  puts("====== member vars =========");

  ref_for<T>([&t](auto p) {
    auto [name, e] = p;
    if constexpr (is_member_object_pointer_v<decltype(e)>) {
      cout << name << "=" << t.*e << endl;
    }
  });
}

template <typename T> struct Func;

template <typename R, typename C, typename... A> struct Func<R (C::*)(A...)> {
  using Args = tuple<A...>;
};

template <typename T> void callMf(T &t) {
  puts("======== member funcs =======");

  ref_for<T>([&t](auto p) {
    auto [name, mf] = p;
    using Tp = decltype(mf);

    if constexpr (is_member_function_pointer_v<Tp>) {
      using Args = typename Func<Tp>::Args;
      Args args{};
      if (tuple_size_v<Args>) {
        tuple_for(args, [&](auto &e) {
          printf("input args for %s(type=%s):", name, typeid(e).name());
          cin >> e;
        });
      }
      apply(mf, tuple_cat(make_tuple(t), args));
    }
  });
}

class Base {
  ref_root(Base);

public:
  void foo() { cout << __FUNCTION__ << endl; }
  ref_field(foo);

  int baseValue = 0;
  ref_field(baseValue);
};

class Child : public Base {
  ref_type(Child);

public:
  static_assert(is_same_v<super, Base>);

  Child() {
    printf("%s:sz:%d,base:%s\n", __name, sizeof(*this), super::__name);
  }

  void foo() {
    printf("%s, data:%d, baseValue:%d\n", __FUNCTION__, data, baseValue);
    data = 2;
  }
  ref_field(foo);

  void bar(int v) {
    printf("%s, param:%d, data:%d\n", __FUNCTION__, v, data);
    data = v;
  }
  ref_field(bar);

private:
  int data = 1;
  ref_field(data);
};

int main() {
  Child g1;
  callMf(g1);
  printMemVar(g1);
  return 0;
}
#endif