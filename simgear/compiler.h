/*compiler.h

This file provided the preprocessor defines needed to compile the properties
code without any dependencies on simgear

*/

#define NOSIMGEAR 1

#define STL_STRING <string>
#define STL_FSTREAM <fstream>
#define STL_IOSTREAM <iostream>

#define SG_LOG(type, level, message) (cout <<message << endl)
#define SG_USING_STD(X) using std::X
