/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGColumnVector3.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGCOLUMNVECTOR3_H
#define FGCOLUMNVECTOR3_H

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
INCLUDES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include <stdlib.h>
#ifdef FGFS
#  include <math.h>
#  include <simgear/compiler.h>
#  include STL_STRING
#  include STL_FSTREAM
#  include STL_IOSTREAM
   SG_USING_STD(string);
   SG_USING_STD(ostream);
   SG_USING_STD(istream);
   SG_USING_STD(cerr);
   SG_USING_STD(cout);
   SG_USING_STD(endl);
#else
#  include <string>
#  if defined(sgi) && !defined(__GNUC__)
#    include <fstream.h>
#    include <math.h>
#    include <iostream.h>
#  else
#    include <fstream>
#    include <cmath>
#    include <iostream>
     using std::ostream;
     using std::istream;
     using std::cerr;
     using std::cout;
     using std::endl;
#  endif
   using std::string;
#endif

#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_COLUMNVECTOR3 "$Id: FGColumnVector3.h,v 1.17 2003/11/26 13:37:36 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class implements a 3 dimensional vector.
    @author Jon S. Berndt, Tony Peden, et. al.
    @version $Id: FGColumnVector3.h,v 1.17 2003/11/26 13:37:36 jberndt Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector3 : public FGJSBBase
{
public:
  FGColumnVector3(void);
  FGColumnVector3(double X, double Y, double Z);
  FGColumnVector3(const FGColumnVector3& b);
  ~FGColumnVector3(void);
  
  FGColumnVector3 operator=(const FGColumnVector3& b);
  
  FGColumnVector3 operator*(const double scalar);
  FGColumnVector3 operator*(const FGColumnVector3& V);   // Cross product operator
  FGColumnVector3 operator/(const double scalar);
  FGColumnVector3 operator+(const FGColumnVector3& B); // must not return reference
  FGColumnVector3 operator-(const FGColumnVector3& B);
  
  void operator-=(const FGColumnVector3 &B);
  void operator+=(const FGColumnVector3 &B);
  void operator*=(const FGColumnVector3 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

  FGColumnVector3& operator<<(const double ff);

  inline void InitMatrix(void) { data[1]=0; data[2]=0; data[3]=0; }
  inline void InitMatrix(double ff) { data[1]=ff; data[2]=ff; data[3]=ff; }

  double Magnitude(void);
  FGColumnVector3 Normalize(void);

  friend FGColumnVector3 operator*(const double scalar, const FGColumnVector3& A);

  friend ostream& operator<<(ostream& os, const FGColumnVector3& col);

  inline double operator()(int m) const { return data[m]; }
  inline double& operator()(int m) { return data[m]; }

  FGColumnVector3 multElementWise(const FGColumnVector3& V);

private:
  double data[4];
  int rowCtr;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
