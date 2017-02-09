/*compiler.h

This file provided the preprocessor defines needed to compile the properties
code without any dependencies on simgear

*/

#define NOSIMGEAR 1

#ifdef _MSC_VER

#define M_PI 3.14159265358979323846
//#define max(a,b) (((a)>(b))?(a):(b))

#define SG_HAVE_STD_INCLUDES

#pragma warning(disable: 4786) // identifier was truncated to '255' characters

#    define STL_IOMANIP    <iomanip>
#    define STL_ITERATOR   <iterator>

#endif

#    define STL_STRING     <string>
#    define STL_FSTREAM    <fstream>
#    define STL_IOSTREAM   <iostream>

#define SG_LOG(type, level, message) (cout <<message << endl)
#define SG_USING_STD(X) using std::X

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

#if defined( __GNUC__ )
#  define DEPRECATED __attribute__ ((deprecated))
#else
#  define DEPRECATED
#endif

#if defined(__clang__)
#  define SG_NO_RETURN [[noreturn]]
#else
#  define SG_NO_RETURN
#endif
