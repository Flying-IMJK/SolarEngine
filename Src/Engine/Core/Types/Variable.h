#pragma once

using int8 = signed char;
using int16 = short;
using int32 = int;
using int64 = long long;

using byte = unsigned char;
using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

namespace SE
{
#define Min_uint8 ((uint8)0x00)
#define	Min_uint16 ((uint16)0x0000)
#define	Min_uint32 ((uint32)0x00000000)
#define Min_uint64 ((uint64)0x0000000000000000)
#define Min_int8 ((int8)-128)
#define Min_int16 ((int16)-32768)
#define Min_int32 -((int32)2147483648)
#define Min_int64 -((int64)9223372036854775808)
#define Min_float -(3.402823466e+38f)
#define Min_double -(1.7976931348623158e+308)
#define Max_uint8 ((uint8)0xff)
#define Max_uint16 ((uint16)0xffff)
#define Max_uint32 ((uint32)0xffffffff)
#define Max_uint64 ((uint64)0xffffffffffffffff)
#define Max_int8 ((int8)127)
#define Max_int16 ((int16)32767)
#define Max_int32 ((int32)2147483647)
#define Max_int64 ((int64)9223372036854775807)
#define Max_float (3.402823466e+38f)
#define Max_double (1.7976931348623158e+308)

#define INVALID_INDEX (-1)

// Unicode text macro
#if PLATFORM_CHAR16
#define _TEXT_(x) u ## x
#else
#define _TEXT_(x) L ## x
#endif
#define SE_TEXT(x) _TEXT_(x)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define __CONCAT_MACROS(a, b) a ## b
#define CONCAT_MACROS(a, b) __CONCAT_MACROS(a, b)
#define __MACRO_TO_STR(x) #x
#define MACRO_TO_STR(x) __MACRO_TO_STR(x)

#define NON_COPYABLE(Type)							\
public:												\
	Type(const Type&) = delete;						\
	const Type& operator=(const Type&) = delete;	\

	// Pointer as integer and pointer size
#ifdef PLATFORM_64BITS
	typedef uint64 uintptr;
	typedef int64 intptr;
#define POINTER_SIZE 8
#else
	typedef uint64 uintptr;
	typedef int64 intptr;
#define POINTER_SIZE 4
#endif

	/// <summary>
/// Defines 16-bit Unicode character for UTF-16 Little Endian.
/// </summary>
#if PLATFORM_CHAR16
	typedef char16_t Char;
#else
	typedef wchar_t Char;
#endif


	class String;
	class StringAnsi;
	class StringView;
	class StringAnsiView;
	struct DateTime;
	class TimeSpan;

	// Forward declarations
	template<typename T>
	class Span;
	class HeapAllocation;

	template<int Capacity>
	class FixedAllocation;
	template<int Capacity, typename OtherAllocator = HeapAllocation>
	class InlinedAllocation;
	template<typename T, typename AllocationType = HeapAllocation>
	class List;
	template<typename KeyType, typename ValueType, typename AllocationType = HeapAllocation>
	class Dictionary;

	template<typename ReturnType, typename... Params>
	class Function;
	template<typename... Params>
	class Delegate;

	// Vector2
	template<typename T> struct Vector2Base;
	typedef Vector2Base<float> Float2;
	typedef Vector2Base<double> Double2;
	typedef Vector2Base<int32> Int2;

	// Vector3
	template<typename T> struct Vector3Base;
	typedef Vector3Base<float> Float3;
	typedef Vector3Base<double> Double3;
	typedef Vector3Base<int32> Int3;

	// Vector4
	template<typename T> struct Vector4Base;
	typedef Vector4Base<float> Float4;
	typedef Vector4Base<double> Double4;
	typedef Vector4Base<int32> Int4;

	struct Quaternion;
	struct Color32;
	struct Color;
	struct Matrix;
	struct Transform;
	struct Rectangle;

	template<class T>
	static T GetZero()
	{
		return T();
	}

}