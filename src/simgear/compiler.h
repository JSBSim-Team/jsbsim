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
