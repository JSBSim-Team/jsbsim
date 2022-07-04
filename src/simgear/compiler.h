/**************************************************************************
 * compiler.h -- C++ Compiler Portability Macros
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 *
 **************************************************************************/

/** \file compiler.h
 * A set of defines to encapsulate compiler and platform differences.
 * Please refer to the source code for full documentation on this file.
 *
 * This file is useful to set compiler-specific options in every file - for
 * example, disabling warnings.
 *
 */

#ifndef _SG_COMPILER_H
#define _SG_COMPILER_H

/*
 * Helper macro SG_STRINGIZE:
 * Converts the parameter X to a string after macro replacement
 * on X has been performed.
 */
#define SG_STRINGIZE(X) SG_DO_STRINGIZE(X)
#define SG_DO_STRINGIZE(X) #X

#ifdef __GNUC__
#  define SG_GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#  define SG_COMPILER_STR "GNU C++ version " SG_STRINGIZE(__GNUC__) "." SG_STRINGIZE(__GNUC_MINOR__)
#endif // __GNUC__

// Dummy SG_LOG macro
#define SG_LOG(type, level, message) (cout << message << endl)

//
// Microsoft compilers.
//
#ifdef _MSC_VER
#  define strcasecmp stricmp

#  if _MSC_VER >= 1200 // msvc++ 6.0 up to MSVC2013
#    if _MSC_VER < 1900
#      define bcopy(from, to, n) memcpy(to, from, n)
#      define snprintf _snprintf
#      define strdup _strdup
#      define copysign _copysign
#    endif

#    pragma warning(disable: 4786) // identifier was truncated to '255' characters
#    pragma warning(disable: 4244) // conversion from double to float
#    pragma warning(disable: 4305) // truncation from larger type to smaller
#    pragma warning(disable: 4267) // conversion from size_t to int / 32-bit type
#    pragma warning(disable: 4996) // don't require _ prefix for standard library functions
#    pragma warning(disable: 4800) // don't warn about int -> bool performance

#  else
#    error What version of MSVC++ is this?
#  endif
#  define SG_COMPILER_STR "Microsoft Visual C++ version " SG_STRINGIZE(_MSC_VER)

#endif // _MSC_VER

//
// Intel C++ Compiler
//
#if defined(__ICC) || defined (__ECC)
#  define SG_COMPILER_STR "Intel C++ version " SG_STRINGIZE(__ICC)
#endif // __ICC

//
// Platform dependent gl.h and glut.h definitions
//

#ifdef __APPLE__
#  define SG_MAC
#  define SG_UNIX
#endif

#if defined (__FreeBSD__)
#  define SG_UNIX
#endif

#if defined (__CYGWIN__)
#  define SG_WINDOWS
#  define SG_UNIX
#  include <ieeefp.h>		// isnan
#endif

// includes both MSVC and mingw compilers
#if defined(_WIN32) || defined(__WIN32__)
#  define SG_WINDOWS
#endif

#if defined(__linux__) || defined(_AIX) || defined ( sgi )
#  define SG_UNIX
#endif

#ifdef __GNUC__
#define SG_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define SG_DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement SG_DEPRECATED for this compiler")
#define SG_DEPRECATED(func) func
#endif

#if defined(__clang__)
#  define SG_NO_RETURN [[noreturn]]
#else
#  define SG_NO_RETURN
#endif

//
// No user modifiable definitions beyond here.
//

#endif // _SG_COMPILER_H
