/* objective.cc
 *
 * Simple examples of how to wrap--or otherwise connect--an
 *   objective function so it can work with the DirectSearch
 *   class.
 *
 * Anne Shepherd, 8/2000 (Thanks to Chris Siefert for the EvalF
 *   function).  Revised, pls, 1/2001
 * Virginia Torczon.  Revised 8/2001.
 *
 * Agostino De Marco 5/2007
 *
 */

#include <math/direct_search/objective.h>

#define ERROR HUGE_VAL

// two handy little functions that make
// use of the C++ conditional operator ? :

inline double max(double i, double j) {
  return (i > j ? i : j);
} // end max
inline int max(int i, int j) {
  return (i > j ? i : j);
} // end max

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//  fcn() is the default function name for
//    DirectSearch.  The three "fcn" functions below are example of how
//    to call different functions in different ways.
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

/* Here, we use fcn to wrap the user's 
 * objective function.  
 */
void fcn(long vars, Vector<double> &x, double & f, bool & flag, void* nothing)
{
    
  // sanity check 
  if (vars < 1) {
      cerr << "\nError: Dimension cannot be less than 1!!\n";
      exit(1);
  }
  // Here's that actual function call:  change this to reflect what
  // you want to do.
  //f = mySin(x);
  f = x[0];
  // This flag can be used to check for success. Here we don't need it.
  flag = true;
  return;
}
