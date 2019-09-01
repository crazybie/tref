#pragma once
#include <assert.h>
#include <string.h>

#define DEF_ENUM2STR

//==========================================================
// str to enum
//==========================================================

// example: EnumStrs(Flip, "FlipNone","FlipV","FlipH")
// NOTE: enum should begin with 0 and auto-increased
#define EnumStrs(Type, ...)                                                    \
  template <> const char *_Enum<Type>::items[] = {__VA_ARGS__, 0};

template <typename T> struct _Enum { static const char *items[]; };

template <typename T> const char *Enum2Str(T e) {
  return _Enum<T>::items[(int)e];
}

template <typename T> T Str2Enum(const char *str, T defValue) {
  int i = 0;
  for (const char **p = _Enum<T>::items; *p; p++, i++) {
    if (!strcmp(str, *p))
      return static_cast<T>(i);
  }
  return defValue;
}

//==========================================================
// flags serialization
//==========================================================

template <typename T, typename Storage = unsigned int> class Flags {
  static_assert(std::is_enum<T>::value == true, "type is not a enum");
  Storage val;

public:
  Flags() : val(0) {}
  void clear() { val = 0; }
  bool hasBit(T e) {
    assert(e < sizeof(val) * 8);
    return (val & (1 << static_cast<Storage>(e))) != 0;
  }
  void setBit(T e) {
    assert(e < sizeof(val) * 8);
    val |= (1 << static_cast<Storage>(e));
  }
  void unsetBit(T e) {
    assert(e < sizeof(val) * 8);
    val &= ~(1 << static_cast<Storage>(e));
  }
};
