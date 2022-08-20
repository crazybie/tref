// Tref: A *T*iny compile time *ref*lection system.

/***********************************************************************
Copyright 2019-2020 crazybie<soniced@sina.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>

#define _TrefHasTref
#define _TrefVersion 0x010000

#ifdef _MSC_VER
#define __TrefCxxVersion _MSVC_LANG
#else
#define __TrefCxxVersion __cplusplus
#endif

#if __TrefCxxVersion < 201700L
#error "Need a c++17 compiler"
#endif

namespace tref {
namespace imp {

using namespace std;

//////////////////////////////////////////////////////////////////////////
//
// Common facility
//
//////////////////////////////////////////////////////////////////////////

template <typename T>
struct Type {
  using type = T;
};

template <size_t L, size_t... R>
constexpr auto tail(index_sequence<L, R...>) {
  return index_sequence<R...>();
};

// member pointer trait

template <class C>
struct member_pointer_trait {
  using enclosing_class_t = void;
  using member_t = void;
};

template <class T, class C>
struct member_pointer_trait<T C::*> {
  using member_t = T;
  using enclosing_class_t = C;
};

template <class T>
using member_t = typename member_pointer_trait<T>::member_t;

template <class T>
using enclosing_class_t = typename member_pointer_trait<T>::enclosing_class_t;

// function trait

template <typename T>
struct func_trait : func_trait<decltype(&T::operator())> {};

template <typename R, typename C, typename... A>
struct func_trait<R (C::*)(A...) const> {
  using args_t = tuple<A...>;
  static constexpr auto args_count = sizeof...(A);
  using ret_t = R;
};

template <typename R, typename C, typename... A>
struct func_trait<R (C::*)(A...)> {
  using args_t = tuple<A...>;
  static constexpr auto args_count = sizeof...(A);
  using ret_t = R;
};

// function overloading helper

template <typename... Args>
struct Overload {
  template <typename R, typename T>
  constexpr auto operator()(R (T::*ptr)(Args...)) const {
    return ptr;
  }
  template <typename R, typename T>
  constexpr auto operator()(R (T::*ptr)(Args...) const) const {
    return ptr;
  }
  template <typename R>
  constexpr auto operator()(R (*ptr)(Args...)) const {
    return ptr;
  }
};

template <typename... Args>
constexpr Overload<Args...> overload_v{};

//  common macros

#define _TrefReturn(...) \
  ->decltype(__VA_ARGS__) { return __VA_ARGS__; }

#define _TrefMsvcExpand(...) __VA_ARGS__
#define _TrefDelay(X, ...) _TrefMsvcExpand(X(__VA_ARGS__))
#define _TrefDelay2(X, ...) _TrefMsvcExpand(X(__VA_ARGS__))
#define _TrefFirst(...) _TrefMsvcExpand(_TrefFirst2(__VA_ARGS__))
#define _TrefFirst2(A, ...) A
#define _TrefSecond(...) _TrefMsvcExpand(_TrefSecond2(__VA_ARGS__))
#define _TrefSecond2(A, B, ...) B
#define _TrefTail(A, ...) __VA_ARGS__
#define _TrefStringify(...) _TrefStringify2(__VA_ARGS__)
#define _TrefStringify2(...) #__VA_ARGS__

#define _TrefRemoveParen(A) \
  _TrefDelay(_TrefRemoveParen2, _TrefRemoveParenHelper A)
#define _TrefRemoveParen2(...) \
  _TrefDelay2(_TrefTail, _TrefRemoveParenHelper##__VA_ARGS__)
#define _TrefRemoveParenHelper(...) _, __VA_ARGS__
#define _TrefRemoveParenHelper_TrefRemoveParenHelper _,

#define _TrefFirstRemoveParen(X) _TrefFirst(_TrefRemoveParen(X))
#define _TrefSecondRemoveParen(X) _TrefSecond(_TrefRemoveParen(X))

// macro version of map

#define _TrefMap(f, arg1, ...)                                         \
  _TrefMsvcExpand(_TrefDelay(_TrefChooseMap, _TrefCount(__VA_ARGS__))( \
      f, arg1, __VA_ARGS__))

#define _TrefChooseMap(N) _TrefMap##N

#define _TrefMap1(m, a, x) m(a, x)
#define _TrefMap2(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap1(m, a, __VA_ARGS__))
#define _TrefMap3(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap2(m, a, __VA_ARGS__))
#define _TrefMap4(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap3(m, a, __VA_ARGS__))
#define _TrefMap5(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap4(m, a, __VA_ARGS__))
#define _TrefMap6(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap5(m, a, __VA_ARGS__))
#define _TrefMap7(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap6(m, a, __VA_ARGS__))
#define _TrefMap8(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap7(m, a, __VA_ARGS__))
#define _TrefMap9(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap8(m, a, __VA_ARGS__))
#define _TrefMap10(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap9(m, a, __VA_ARGS__))
#define _TrefMap11(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap10(m, a, __VA_ARGS__))
#define _TrefMap12(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap11(m, a, __VA_ARGS__))
#define _TrefMap13(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap12(m, a, __VA_ARGS__))
#define _TrefMap14(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap13(m, a, __VA_ARGS__))
#define _TrefMap15(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap14(m, a, __VA_ARGS__))
#define _TrefMap16(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap15(m, a, __VA_ARGS__))
#define _TrefMap17(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap16(m, a, __VA_ARGS__))
#define _TrefMap18(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap17(m, a, __VA_ARGS__))
#define _TrefMap19(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap18(m, a, __VA_ARGS__))
#define _TrefMap20(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap19(m, a, __VA_ARGS__))
#define _TrefMap21(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap20(m, a, __VA_ARGS__))
#define _TrefMap22(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap21(m, a, __VA_ARGS__))
#define _TrefMap23(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap22(m, a, __VA_ARGS__))
#define _TrefMap24(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap23(m, a, __VA_ARGS__))
#define _TrefMap25(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap24(m, a, __VA_ARGS__))
#define _TrefMap26(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap25(m, a, __VA_ARGS__))
#define _TrefMap27(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap26(m, a, __VA_ARGS__))
#define _TrefMap28(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap27(m, a, __VA_ARGS__))
#define _TrefMap29(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap28(m, a, __VA_ARGS__))
#define _TrefMap30(m, a, x, ...) \
  m(a, x) _TrefMsvcExpand(_TrefMap29(m, a, __VA_ARGS__))

#define _TrefEvaluateCount(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                           _13, _14, _15, _16, _17, _18, _19, _20, _21, _22,  \
                           _23, _24, _25, _26, _27, _28, _29, _30, N, ...)    \
  N

#define _TrefCount(...)                                                        \
  _TrefMsvcExpand(_TrefEvaluateCount(                                          \
      __VA_ARGS__, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, \
      15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1))

//////////////////////////////////////////////////////////////////////////
//
// Core facility
//
//////////////////////////////////////////////////////////////////////////

template <int N = 255>
struct Id : Id<N - 1> {
  enum { value = N };
};

template <>
struct Id<0> {
  enum { value = 0 };
};

constexpr auto invalid_index = 0;

template <typename C, typename Tag>
tuple<Id<invalid_index>> _tref_state(C**, Tag, Id<0> id);

// use macro to delay the evaluation
#define _TrefStateCnt(C, Tag)                                                \
  std::tuple_element_t<0,                                                    \
                       decltype(_tref_state((_TrefRemoveParen(C)**)0, Tag{}, \
                                            tref::imp::Id<>{}))>::value

#define _TrefStatePush(C, Tag, ...)                                       \
  constexpr auto _tref_state(_TrefRemoveParen(C)**, Tag,                  \
                             tref::imp::Id<_TrefStateCnt(C, Tag) + 1> id) \
      _TrefReturn(std::tuple(id, __VA_ARGS__))

template <class C, class Tag, int idx>
constexpr auto get_state() {
  return _tref_state((C**)0, Tag{}, Id<idx>{});
}

template <class C, class Tag, class F, size_t... Is>
constexpr bool state_fold(index_sequence<Is...>, F&& f) {
  return (f(get<1>(get_state<C, Tag, Is>())) && ...);
}

template <typename C, typename Tag, typename F>
constexpr bool each_state(F f) {
  constexpr auto cnt = _TrefStateCnt(C, Tag);
  if constexpr (cnt > 0) {
    return state_fold<C, Tag>(tail(make_index_sequence<cnt + 1>{}), f);
  } else
    return true;
};

//////////////////////////////////////////////////////////////////////////
//
// class reflection
//
//////////////////////////////////////////////////////////////////////////

struct DummyBase;

struct FieldTag {};
struct MemberTypeTag {};
struct SubclassTag {};

void _tref_class_info(void*);

template <typename T>
constexpr auto is_reflected_v =
    !std::is_same_v<decltype(_tref_class_info((T**)0)), void>;

// Use function to delay the evaluation. (for non-conformance mode of MSVC)
template <typename T>
constexpr auto class_info() {
  return _tref_class_info((T**)0);
};

// Use macro to delay the evaluation. (for non-conformance mode of MSVC)
#define _TrefBaseOf(T) \
  typename decltype(tref::imp::class_info<_TrefRemoveParen(T)>())::base_t

template <typename T>
constexpr auto has_base_class_v =
    is_reflected_v<T> ? !std::is_same_v<_TrefBaseOf(T), DummyBase> : false;

// Meta for Member

template <typename T, typename Meta>
struct FieldInfo {
  using enclosing_class_t = imp::enclosing_class_t<T>;
  using member_t = imp::member_t<T>;

  static constexpr auto is_member_v = !is_same_v<enclosing_class_t, void>;

  int         index;
  string_view name;

  // Possible values:
  // 1. Address of member variables & functions
  // 2. Type<T> for member type T: use decltype(value)::type to retrive it.
  T    value;
  Meta meta;

  constexpr FieldInfo(int idx, string_view n, T a, Meta m)
      : index{idx}, name{n}, value{a}, meta{m} {}
};

// Meta for class

template <typename T, typename Base, typename Meta>
struct ClassInfo {
  using class_t = T;
  using base_t = Base;

  string_view name;
  size_t      size;
  Type<Base>  base;
  Meta        meta;

  constexpr ClassInfo(T*, string_view n, size_t sz, Type<Base> b, Meta&& m)
      : name{n}, size{sz}, base{b}, meta{move(m)} {
    if constexpr (!is_same_v<Base, DummyBase>) {
      static_assert(is_base_of_v<Base, class_t>, "invalid base class");
    }
  }

  template <typename Tag, typename F>
  constexpr bool each_r(F&& f, int level = 0) const {
    auto next =
        each_state<T, Tag>([&](const auto& info) { return f(info, level); });
    if (next)
      if constexpr (!is_same_v<Base, DummyBase>)
        return class_info<Base>().template each_r<Tag>(f, level + 1);
    return next;
  }

  // Iterate through the members recursively.
  // @param f: [](MemberInfo info, int level) -> bool, return false to stop the
  // iterating.
  template <typename F>
  constexpr bool each_field(F&& f) const {
    return each_r<FieldTag>(f);
  }

  constexpr int get_field_index(string_view name) const {
    int idx = invalid_index;
    each_field([&](auto info, int) {
      if (info.name == name) {
        idx = info.index;
        return false;
      }
      return true;
    });
    return idx;
  }

  template <int index>
  constexpr auto get_field() const {
    return get<1>(get_state<T, FieldTag, index>());
  }

  // Iterate through the subclasses recursively.
  // @param F: [](ClassInfo info, int level) -> bool, return false to stop the
  // iterating.
  template <typename F>
  constexpr bool each_subclass(F&& f, int level = 0) const {
    return each_state<T, SubclassTag>([&](auto info) {
      using S = typename decltype(info)::type;
      return f(class_info<S>(), level) &&
             class_info<S>().each_subclass(f, level + 1);
    });
  }

  // Iterate through the member types.
  // @param F: [](MemberInfo info) -> bool, return false to stop the
  // iterating.
  template <typename F>
  constexpr bool each_member_type(F&& f) const {
    return each_r<MemberTypeTag>(f);
  }
};

#define _TrefClassMetaImp(T, Base, meta)                              \
  constexpr auto _tref_class_info(_TrefRemoveParen(T)**) {            \
    return tref::imp::ClassInfo{                                      \
        (_TrefRemoveParen(T)*)0, _TrefStringify(_TrefRemoveParen(T)), \
        sizeof(_TrefRemoveParen(T)),                                  \
        tref::imp::Type<_TrefRemoveParen(Base)>{}, meta};             \
  }

#define _TrefClassMeta(T, Base, meta) friend _TrefClassMetaImp(T, Base, meta)

#define _TrefPushFieldImp(T, Tag, name, val, meta) \
  _TrefStatePush(T, Tag, tref::imp::FieldInfo{id.value, name, val, meta})

//////////////////////////////////////////////////////////////////////////
//
// macros for class reflection
//
//////////////////////////////////////////////////////////////////////////

// sub type

template <typename T, class = void_t<>>
struct get_parent {
  using type = DummyBase;
};

template <typename T>
struct get_parent<T, void_t<typename T::__parent_t>> {
  using type = typename T::__parent_t;
};

#define _TrefSubType(T) \
  _TrefSubTypeImp(T, typename tref::imp::get_parent<_TrefRemoveParen(T)>::type)

#define _TrefSubTypeImp(T, Base)                                     \
  _TrefStatePush(Base, tref::imp::SubclassTag, tref::imp::Type<T>{}) \
      _TrefAllowSemicolon(T)

// fix lint issue: `TrefSubType(T);` : empty statement.
#define _TrefAllowSemicolon(T) using T = T

//

#define _TrefTypeCommon(T, Base, meta) \
 private:                              \
  using self_t = _TrefRemoveParen(T);  \
                                       \
 public:                               \
  using __parent_t = self_t;           \
  _TrefClassMeta(T, Base, meta);

// Just reflect the type.

#define _TrefType(T) _TrefTypeWithMeta(T, nullptr)
#define _TrefTypeWithMeta(T, meta)                                            \
 private:                                                                     \
  using __base_t = typename tref::imp::get_parent<_TrefRemoveParen(T)>::type; \
  _TrefTypeCommon(T, __base_t, meta);

// reflect member variable & function

#define _TrefField1(t) _TrefFieldWithMeta2(t, nullptr)
#define _TrefFieldWithMeta2(t, meta) _TrefFieldWithMeta2Imp(self_t, t, meta)
#define _TrefFieldWithMeta2Imp(T, t, meta)               \
  _TrefPushFieldImp(T, tref::imp::FieldTag,              \
                    _TrefStringify(_TrefRemoveParen(t)), \
                    &T::_TrefRemoveParen(t), meta)

// provide arguments for overloaded function
#define _TrefField2(t, sig) _TrefFieldWithMeta3(t, sig, nullptr)
#define _TrefFieldWithMeta3(t, sig, meta) \
  _TrefFieldWithMeta3Imp(self_t, t, sig, meta)

#define _TrefFieldWithMeta3Imp(T, t, sig, meta)                              \
  _TrefPushFieldImp(                                                         \
      T, tref::imp::FieldTag, _TrefStringify(_TrefRemoveParen(t)),           \
      tref::imp::overload_v<_TrefRemoveParen(sig)>(&T::_TrefRemoveParen(t)), \
      meta)

// auto select from _TrefField1 or _TrefField2 by argument count
#define _TrefFieldImp(...) \
  _TrefMsvcExpand(         \
      _TrefDelay(_TrefChooseField, _TrefCount(__VA_ARGS__))(__VA_ARGS__))
#define _TrefChooseField(N) _TrefField##N

#define _TrefField(...) friend _TrefFieldImp(__VA_ARGS__)

// auto select from _TrefFieldWithMeta2 or _TrefFieldWithMeta3 by argument
// count
#define _TrefFieldWithMetaImp(...)                     \
  _TrefMsvcExpand(_TrefDelay(_TrefChooseFieldWithMeta, \
                             _TrefCount(__VA_ARGS__))(__VA_ARGS__))
#define _TrefChooseFieldWithMeta(N) _TrefFieldWithMeta##N

#define _TrefFieldWithMeta(...) friend _TrefFieldWithMetaImp(__VA_ARGS__)

// reflect member type
#define _TrefMemberTypeImp(T) _TrefMemberTypeWithMetaImp(T, nullptr)
#define _TrefMemberTypeWithMetaImp(T, meta)              \
  _TrefPushFieldImp(self_t, tref::imp::MemberTypeTag,    \
                    _TrefStringify(_TrefRemoveParen(T)), \
                    tref::imp::Type<_TrefRemoveParen(T)>{}, meta)

#define _TrefMemberType(T) friend _TrefMemberTypeImp(T)
#define _TrefMemberTypeWithMeta(T, meta) \
  friend _TrefMemberTypeWithMetaImp(T, meta)

//////////////////////////////////////////////////////////////////////////
//
// enum reflection
//
//////////////////////////////////////////////////////////////////////////

struct EnumValueConvertor {
  template <typename T>
  constexpr EnumValueConvertor(T v) : value((size_t)v) {
    static_assert(sizeof(T) <= sizeof(value));
  }

  template <typename U>
  constexpr EnumValueConvertor operator=(U) {
    return *this;
  }

  template <typename U>
  constexpr operator U() {
    static_assert(is_enum_v<U>);
    return (U)value;
  }

  size_t value = 0;
};

template <typename T, typename Meta>
struct EnumItem {
  string_view name;
  T           value;
  Meta        meta;
};

template <typename T, size_t N, typename ItemMeta>
using EnumItems = array<EnumItem<T, ItemMeta>, N>;

template <typename T, size_t N, typename Meta, typename ItemMeta>
struct EnumInfo {
  using enum_t = T;

  string_view               name;
  size_t                    size;
  EnumItems<T, N, ItemMeta> items;
  Meta                      meta;

  // @param f: [](string_view name, enum_t val)-> bool, return false to stop
  // the iterating.
  //
  // NOTE: the name.data() is not the name of the item, please use
  // string(name.data(), name.size()).
  template <typename F>
  constexpr auto each_item(F&& f) const {
    for (auto& e : items) {
      if (!f(e))
        return false;
    }
    return true;
  }

  static constexpr auto npos = -1;

  constexpr int index_of_value(T v) {
    auto i = 0;
    for (auto& e : items) {
      if (e.value == v) {
        return i;
      }
      i++;
    }
    return npos;
  }

  constexpr int index_of_name(string_view n) {
    auto i = 0;
    for (auto& e : items) {
      if (e.name == n) {
        return i;
      }
      i++;
    }
    return npos;
  }
};

template <typename T, int N, typename Meta, typename ItemMeta>
constexpr auto makeEnumInfo(string_view                      name,
                            size_t                           size,
                            const EnumItems<T, N, ItemMeta>& items,
                            Meta                             meta) {
  return EnumInfo<T, N, Meta, ItemMeta>{name, size, items, meta};
}

void* _tref_enum_info(void*);

template <typename T, typename = enable_if_t<is_enum_v<T>, bool>>
constexpr auto enum_info() {
  return _tref_enum_info((T**)0);
};

// Use it out of class.
#define _TrefEnum(T, ...) _TrefEnumWithMeta(T, nullptr, __VA_ARGS__)
#define _TrefEnumWithMeta(T, meta, ...) \
  enum class T { __VA_ARGS__ };         \
  _TrefEnumImpWithMeta(T, meta, __VA_ARGS__)

// Use it inside of class.
#define _TrefMemberEnum(T, ...) _TrefMemberEnumWithMeta(T, 0, __VA_ARGS__)
#define _TrefMemberEnumWithMeta(T, meta, ...) \
  enum class T { __VA_ARGS__ };               \
  friend _TrefEnumImpWithMeta(T, meta, __VA_ARGS__)

// Reflect enum items of already defined enum.
#define _TrefEnumImp(T, ...) _TrefEnumImpWithMeta(T, nullptr, __VA_ARGS__)
#define _TrefEnumImpWithMeta(T, meta, ...)                                   \
  constexpr auto _tref_enum_info(_TrefRemoveParen(T)**) {                    \
    return tref::imp::EnumInfo<_TrefRemoveParen(T), _TrefCount(__VA_ARGS__), \
                               decltype(meta), nullptr_t>{                   \
        _TrefStringify(_TrefRemoveParen(T)),                                 \
        sizeof(_TrefRemoveParen(T)),                                         \
        {_TrefEnumStringize(T, __VA_ARGS__)},                                \
        std::move(meta)};                                                    \
  }

#define _TrefEnumStringize(P, ...) \
  _TrefMsvcExpand(_TrefMap(_TrefEnumStringizeSingle, P, __VA_ARGS__))

#define _TrefEnumStringizeSingle(P, E)                                         \
  tref::imp::EnumItem<_TrefRemoveParen(P), nullptr_t>{                         \
      tref::imp::enum_trim_name(#E),                                           \
      (tref::imp::EnumValueConvertor)_TrefRemoveParen(P)::_TrefRemoveParen(E), \
      nullptr},

  //////////////////////////////////////////////////////////////////////////
  // ex version support meta for enum items.

#define _TrefMemberEnumEx(T, ...) _TrefMemberEnumWithMetaEx(T, 0, __VA_ARGS__)
#define _TrefMemberEnumWithMetaEx(T, meta, ...) \
  _TrefEnumDefineEnum2(T, __VA_ARGS__);         \
  friend _TrefEnumImpWithMetaEx(T, meta, __VA_ARGS__)

#define _TrefEnumEx(T, ...) _TrefEnumWithMetaEx(T, nullptr, __VA_ARGS__)

#define _TrefEnumWithMetaEx(T, meta, ...) \
  _TrefEnumDefineEnum2(T, __VA_ARGS__);   \
  _TrefEnumImpWithMetaEx(T, meta, __VA_ARGS__)

#define _TrefFirstArgAsEnumItemDef(P, E) _TrefFirstRemoveParen(E),
#define _TrefEnumDefineEnum2(T, ...)                                      \
  enum class T {                                                          \
    _TrefMsvcExpand(_TrefMap(_TrefFirstArgAsEnumItemDef, T, __VA_ARGS__)) \
  }

#define _TrefEnumImpEx(T, ...) _TrefEnumImpWithMetaEx(T, 0, __VA_ARGS__)
#define _TrefEnumImpWithMetaEx(T, meta, ...)                               \
  constexpr auto _tref_enum_info(_TrefRemoveParen(T)**) {                  \
    return tref::imp::makeEnumInfo<_TrefRemoveParen(T),                    \
                                   _TrefCount(__VA_ARGS__)>(               \
        _TrefStringify(_TrefRemoveParen(T)), sizeof(_TrefRemoveParen(T)),  \
        std::array{_TrefEnumStringize2(T, __VA_ARGS__)}, std::move(meta)); \
  }

#define _TrefEnumStringize2(P, ...) \
  _TrefMsvcExpand(_TrefMap(_TrefEnumStringizeSingle2, P, __VA_ARGS__))

#define _TrefEnumStringizeSingle2(P, E)                                       \
  tref::imp::EnumItem<_TrefRemoveParen(P),                                    \
                      decltype(_TrefRemoveParen(_TrefSecondRemoveParen(E)))>{ \
      tref::imp::enum_trim_name(_TrefStringify(_TrefFirstRemoveParen(E))),    \
      (tref::imp::EnumValueConvertor)_TrefRemoveParen(P)::_TrefRemoveParen(   \
          _TrefFirstRemoveParen(E)),                                          \
      _TrefRemoveParen(_TrefSecondRemoveParen(E))},

/////////////////////////////////////

constexpr string_view enum_trim_name(string_view s) {
  auto p = s.find_first_of('=');
  if (p != string_view::npos)
    p = s.rfind(' ', p);
  return s.substr(0, p);
}

template <typename T>
constexpr auto enum_to_string(T v) {
  static_assert(is_enum_v<T>);
  for (auto& e : enum_info<T>().items) {
    if (e.value == v) {
      return e.name;
    }
  }
  return string_view{};
}

template <typename T>
constexpr auto string_to_enum(string_view s, T default_) {
  static_assert(is_enum_v<T>);
  for (auto& e : enum_info<T>().items) {
    if (s == e.name) {
      return e.value;
    }
  }
  return default_;
}

template <typename T, typename Storage = unsigned int>
struct Flags {
  static_assert(std::is_enum_v<T>);
  Storage value = 0;

  constexpr Flags() {}
  constexpr void clear() { value = 0; }
  constexpr bool hasFlag(T e) {
    assert(e < sizeof(value) * 8);
    return (value & (1 << static_cast<Storage>(e))) != 0;
  }
  constexpr void setFlag(T e) {
    assert(e < sizeof(value) * 8);
    value |= (1 << static_cast<Storage>(e));
  }
  constexpr void clearFlag(T e) {
    assert(e < sizeof(value) * 8);
    value &= ~(1 << static_cast<Storage>(e));
  }
};

}  // namespace imp

//////////////////////////////////////////////////////////////////////////
//
// public APIs
//
//////////////////////////////////////////////////////////////////////////

#define TrefHasTref _TrefHasTref
#define TrefVersion _TrefVersion

using imp::class_info;
using imp::ClassInfo;
using imp::enclosing_class_t;
using imp::FieldInfo;
using imp::func_trait;
using imp::has_base_class_v;
using imp::is_reflected_v;
using imp::member_t;
using imp::overload_v;

#define TrefType _TrefType
#define TrefTypeWithMeta _TrefTypeWithMeta
#define TrefSubType _TrefSubType
#define TrefBaseOf _TrefBaseOf

#define TrefField _TrefField
#define TrefFieldWithMeta _TrefFieldWithMeta
#define TrefMemberType _TrefMemberType
#define TrefMemberTypeWithMeta _TrefMemberTypeWithMeta

//////////////////////////
// Reflect external types
// NOTE: not support template.
//////////////////////////
#define TrefNoBase tref::imp::DummyBase
#define TrefExternalTypeWithMeta _TrefClassMetaImp
#define TrefExternalFieldWithMeta _TrefFieldWithMeta2Imp
#define TrefExternalOverloadedFieldWithMeta _TrefFieldWithMeta3Imp
#define TrefExternalSubType _TrefSubTypeImp

/// enum

using imp::enum_info;
using imp::enum_to_string;
using imp::Flags;
using imp::string_to_enum;

// ex version support meta for enum items.

#define TrefEnum _TrefEnum
#define TrefEnumEx _TrefEnumEx
#define TrefEnumWithMeta _TrefEnumWithMeta
#define TrefEnumWithMetaEx _TrefEnumWithMetaEx
#define TrefMemberEnum _TrefMemberEnum
#define TrefMemberEnumEx _TrefMemberEnumEx
#define TrefMemberEnumWithMeta _TrefMemberEnumWithMeta
#define TrefMemberEnumWithMetaEx _TrefMemberEnumWithMetaEx
#define TrefExternalEnum _TrefEnumImp
#define TrefExternalEnumEx _TrefEnumImpEx
#define TrefExternalEnumWithMeta _TrefEnumImpWithMeta
#define TrefExternalEnumWithMetaEx _TrefEnumImpWithMetaEx

}  // namespace tref
