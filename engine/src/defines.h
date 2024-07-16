#pragma once

// Unsigned int types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int Types
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef _Bool b8;

/** @brief A range, typically of memory */
typedef struct range {
    /** @brief The offset in bytes. */
    u64 offset;
    /** @brief The size in bytes. */
    u64 size;
} range;

// Properly define static assertions
#if defined(__clang__) || defined(__GNUC__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes");

STATIC_ASSERT(sizeof(b32) == 4, "Expected b32 to be 4 bytes");
STATIC_ASSERT(sizeof(b8) == 1, "Expected b32 to be 1 byte");

#define true 1
#define false 0

// generic methods
#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object. 
 */
#define INVALID_ID_U64 18446744073709551615UL
#define INVALID_ID 4294967295U
#define INVALID_ID_U16 65535U
#define INVALID_ID_U8 255U

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define PLATFORM_LINUX 1
#if defined(__ANDROID__)
#define PLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define PLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define PLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// IOS Simulator
#define PLATFORM_IOS 1
#define PLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define PLATFORM_IOS 1
// IOS device
#elif TARGET_OS_MAC
// Other kind of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define API __declspec(dllimport)
#else
#define API
#endif
#endif

#define CLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max \
                                                                      : value)

// Inlining
#ifdef _MSC_VER
#define INLINE __forceinline
#define NOINLINE __declspec(noinline)
#else
#define INLINE static inline
#define NOINLINE
#endif

// GiB
#define GIBIBYTES(amount) (amount * 1024ULL * 1024ULL * 1024ULL)
// MiB
#define MEBIBYTES(amount) (amount * 1024ULL * 1024ULL)
// KiB
#define KIBIBYTES(amount) (amount * 1024ULL)
// GB
#define GIGABYTES(amount) (amount * 1000ULL * 1000ULL * 1000ULL)
// GB
#define MEGABYTES(amount) (amount * 1000ULL * 1000ULL)
// GB
#define KIGABYTES(amount) (amount * 1000ULL)

INLINE u64 get_aligned(u64 operand, u64 granularity) {
    return ((operand + (granularity - 1)) & ~(granularity - 1));
}

INLINE range get_aligned_range(u64 offset, u64 size, u64 granularity) {
    return (range){get_aligned(offset, granularity), get_aligned(size, granularity)};
}

#define KMIN(x, y) (x < y ? x : y)
#define KMAX(x, y) (x > y ? x : y)