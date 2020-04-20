#pragma once
#include <tuple>

#define TREF

/// refs: https://woboq.com/blog/verdigris-implementation-tricks.html
namespace tref {

using namespace std;

struct IndexBase {};

template <size_t>
struct Index : IndexBase {};

template <class ItemTag, class ClassTag>
void w_state(IndexBase, ItemTag, ClassTag);

template <int L, class ItemTag, class ClassTag, size_t N, size_t M>
constexpr size_t countBetween() {
  constexpr size_t X = (N + M) / 2;
  using R = decltype(w_state(Index<X>{}, ItemTag{}, ClassTag{}));

  if constexpr (N == X) {
    return is_same_v<void, R> ? N : M;
  } else if constexpr (is_same_v<void, R>) {
    return countBetween<L, ItemTag, ClassTag, N, X>();
  } else {
    return countBetween<L, ItemTag, ClassTag, X, M>();
  }
}

template <int L, class ItemTag, class ClassTag, size_t N = 1>
constexpr size_t stateCount() {
  using R = decltype(w_state(Index<N>{}, ItemTag{}, ClassTag{}));
  if constexpr (is_same_v<void, R>) {
    return countBetween<L, ItemTag, ClassTag, N / 2, N>();
  } else {
    return stateCount<L, ItemTag, ClassTag, N * 2>();
  }
}

template <class F, size_t... Is>
constexpr bool fold(index_sequence<Is...>, F&& f) {
  return (f(Index<Is>{}) && ...);
}

template <typename T, typename ItemTag, typename F>
constexpr bool each(F f) {
  auto indexes = make_index_sequence<stateCount<1000, ItemTag, T**>()>{};
  return fold(indexes, [&](auto idx) {
    return f(w_state(idx, ItemTag{}, (T**)nullptr));
  });
}

template <int N, typename ItemTag, typename ClassTag>
using NextIndex = Index<stateCount<N, ItemTag, ClassTag>()>;

#define _RefReturn(T) \
  ->decltype(T) { return T; }

#define _RefPush(T, I, ...)                                                   \
  friend constexpr auto w_state(tref::NextIndex<__COUNTER__, I, T**>, I, T**) \
      _RefReturn(__VA_ARGS__);

//////////////////////////////////////////////////////////////////////////

struct MemberTag {};
struct SubclassTag {};

#define RefTypeRoot(T) _RefCommon(T);

#define RefType(T)       \
  using super = _parent; \
  _RefCommon(T);         \
  _RefPush(super, tref::SubclassTag, (self*)0);

#define _RefCommon(T) \
  using self = T;     \
  struct RefTag;      \
                      \
 protected:           \
  using _parent = T;  \
                      \
 public:              \
  static constexpr auto __meta = std::make_tuple(#T, __FILE__, __LINE__);

#define _Reflected(F, val, ...) \
  _RefPush(self, tref::MemberTag, std::make_tuple(#F, val, __VA_ARGS__))

#define RefMemberWithMeta(F, ...) _Reflected(F, &self::F, __VA_ARGS__)
#define RefMember(F) RefMemberWithMeta(F, nullptr)
#define RefMemberTypeWithMeta(F, ...) _Reflected(F, (F**)0, __VA_ARGS__)
#define RefMemberType(F, ...) RefMemberTypeWithMeta(F, nullptr)

//////////////////////////////////////////////////////////////////////////

#define _RefDefChecker(name, expr)        \
  template <typename T, class = void_t<>> \
  struct name : false_type {};            \
                                          \
  template <typename T>                   \
  struct name<T, void_t<decltype((expr*)0)>> : true_type {};

_RefDefChecker(IsReflected, typename T::RefTag);
_RefDefChecker(HasSuper, typename T::super);

/// recursive
template <class T, typename F>
constexpr bool eachMember(F&& f, int level = 0) {
  auto next = each<T, MemberTag>([&](auto& c) {
    auto [name, ptr, meta] = c;
    return f(name, ptr, meta, level);
  });
  if (next)
    if constexpr (HasSuper<T>::value)
      return eachMember<typename T::super>(f, level + 1);
  return true;
}

/// recursive
template <class T, typename F>
constexpr bool eachSubclass(F&& f, int level = 0) {
  return each<T, SubclassTag>([&](auto* c) {
    using C = remove_pointer_t<decltype(c)>;
    return f(c, level) && eachSubclass<C>(f, level + 1);
  });
}

}  // namespace tref
