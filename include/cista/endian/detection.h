#pragma once

// Based on:
// https://stackoverflow.com/a/32210953 (MSVC)
// https://stackoverflow.com/a/27054190 (GCC/Clang)

#if !defined(CISTA_BIG_ENDIAN) && !defined(CISTA_LITTLE_ENDIAN)

#if defined(__APPLE__)
#include <machine/endian.h>
#elif defined(__GNUC__)
#include <endian.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#if defined(REG_DWORD) && REG_DWORD == REG_DWORD_BIG_ENDIAN ||               \
    defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                 \
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||       \
    defined(__MIBSEB__)
#define CISTA_BIG_ENDIAN
#elif defined(REG_DWORD) && REG_DWORD == REG_DWORD_LITTLE_ENDIAN ||       \
    defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN ||           \
    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) ||                   \
    defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || \
    defined(__MIPSEL) || defined(__MIPSEL__)
#define CISTA_LITTLE_ENDIAN
#else
#error "architecture: unknown byte order"
#endif

#endif
