//objective.h
//this example file declares the functions as required
// Liz Dolan, P.L. (Anne) Shepherd, Virginia Torczon
// College of William and Mary
//
// Modified by Agostino De Marco (many functions deleted)
//
/* 
    References:

    Torczon, V.; Dolan, L.; Gurson, A.; Shepherd, A.; Siefert, C., Yates, 
    A.: C++ > DirectSearch Classes. Software available at 
    http://www.cs.wm.edu/~va/software/DirectSearch/

    Shepherd, P. L.: Class Documentation for DirectSearch and its derived 
    classes.
    Available at http://www.cs.wm.edu/~plshep/

*/

#if !defined _userfile_
#define _userfile_


#include <math/direct_search/vec.h>
#if defined(sgi) && !defined(__GNUC__)
# include <math.h>
#else
# include <cmath>
#endif
#include <iostream>

#if defined(_MSC_VER)
	//#include <io.h>

#else
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/types.h>
#endif

#include <stdio.h>


//Chris Siefert's Bound struct--used for constrained problems
struct Bound {
  Vector<double> lower;
  Vector<double> upper;
};

void fcn(long, Vector<double> &, double &, bool &, void *);

#endif


