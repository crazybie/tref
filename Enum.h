#pragma once
#include <string.h>
#include <assert.h>


//==========================================================
// str to enum
//==========================================================

// example: EnumStrs(Flip, "FlipNone","FlipV","FlipH")
// NOTE: enum should begin with 0 and auto-increased
#define EnumStrs(Type, ...) \
    template<> const char* _Enum<Type>::items[] = { __VA_ARGS__, 0 };

template<typename T>
struct _Enum
{
    static const char* items[];
};

template<typename T>
const char* Enum2Str(T e) { return _Enum<T>::items[(int)e]; }

template<typename T>
T Str2Enum(const char* str, T defValue)
{
    int i = 0;
    for ( const char** p = _Enum<T>::items; *p; p++, i++ ) {
        if ( !strcmp(str, *p) )
            return static_cast<T>( i );
    }
    return defValue;
}

#ifdef JSON_READER

template<typename T>
typename std::enable_if<std::is_enum<T>::value, bool>::type
operator >> (JsonReader& in, T& v)
{
    std::string s;
    if ( !( in >> s ) ) return false;
    v = Str2Enum(s.c_str(), T(-1));
    return v != T(-1);
}
#endif

//==========================================================
// flags serialization
//==========================================================

template<typename T, typename Storage = unsigned int>
class Flags
{
    static_assert( std::is_enum<T>::value == true, "type is not a enum" );
    Storage val;
public:
    Flags() :val(0) {}
    void clear() { val = 0; }
    bool hasBit(T e) { assert(e < sizeof(val) * 8); return ( val & ( 1 << static_cast<Storage>( e ) ) ) != 0; }
    void setBit(T e) { assert(e < sizeof(val) * 8); val |= ( 1 << static_cast<Storage>( e ) ); }
    void unsetBit(T e) { assert(e < sizeof(val) * 8); val &= ~( 1 << static_cast<Storage>( e ) ); }
};

#ifdef JSON_READER

template<typename T>
bool operator >> (JsonReader& in, Flags<T>& v)
{
    if ( !in.expectArrayStart() )return false;
    for ( v.clear(); !in.expectArrayEnd();) {
        T t;
        if ( in >> t ) { v.setBit(t); } else return false;
    }
    return true;
}

#endif