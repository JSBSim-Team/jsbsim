/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGColumnVector4.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
SENTRY
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#ifndef FGCOLUMNVECTOR4_H
#define FGCOLUMNVECTOR4_H

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
#  if defined (sgi) && !defined(__GNUC__) && (_COMPILER_VERSION < 740)
#    include <fstream.h>
#    include <iostream.h>
#    include <math.h>
#  else
#    include <fstream>
#  if defined (sgi) && !defined(__GNUC__)
#    include <math.h>
#  else
#    include <cmath>
#  endif
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

#define ID_COLUMNVECTOR4 "$Id: FGColumnVector4.h,v 1.16 2003/12/29 10:57:39 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DOCUMENTATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/** This class implements a 4 dimensional vector.
    @author Jon S. Berndt, Tony Peden, et. al.
    @version $Id: FGColumnVector4.h,v 1.16 2003/12/29 10:57:39 ehofman Exp $
*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
CLASS DECLARATION
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector4 : public FGJSBBase
{
public:
  FGColumnVector4(void);
  FGColumnVector4(double A, double B, double C, double D);
  FGColumnVector4(const FGColumnVector4& b);
  ~FGColumnVector4(void);
  
  FGColumnVector4 operator=(const FGColumnVector4& b);
  
  FGColumnVector4 operator*(const double scalar);
  FGColumnVector4 operator/(const double scalar);
  FGColumnVector4 operator+(const FGColumnVector4& B); // must not return reference
  FGColumnVector4 operator-(const FGColumnVector4& B);
  
  void operator-=(const FGColumnVector4 &B);
  void operator+=(const FGColumnVector4 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);
  
  inline double operator()(int m) const { return data[m]; }
  inline double& operator()(int m) { return data[m]; }
  
  FGColumnVector4& operator<<(const double ff);

  inline void InitMatrix(void) { data[1]=0; data[2]=0; data[3]=0; data[4]=0; }
  inline void InitMatrix(double ff) { data[1]=ff; data[2]=ff; data[3]=ff; data[4]=ff;}

  double Magnitude(void);
  FGColumnVector4 Normalize(void);

  friend FGColumnVector4 operator*(const double scalar, const FGColumnVector4& A);

  friend ostream& operator<<(ostream& os, FGColumnVector4& col);


  FGColumnVector4 multElementWise(const FGColumnVector4& V);

private:
  double data[5];
  int rowCtr;
  void Debug(int from);
};
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif

