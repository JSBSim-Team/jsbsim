/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

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
#  if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
     SG_USING_STD(ostream);
     SG_USING_STD(istream);
     SG_USING_STD(cerr);
     SG_USING_STD(cout);
     SG_USING_STD(endl);
#  endif
#else
#  include <fstream>
#  include <cmath>
#  include <iostream>
#  include <string>
   using std::string;
   using std::ostream;
   using std::istream;
   using std::cerr;
   using std::cout;
   using std::endl;
#endif

#include "FGMatrix33.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_COLUMNVECTOR3 "$Id: FGColumnVector3.h,v 1.3 2001/07/24 11:55:49 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix33;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGColumnVector3
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector3
{
public:
  FGColumnVector3(void);
  FGColumnVector3(int m);
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

  
  float Magnitude(void);
  FGColumnVector3 Normalize(void);

  friend FGColumnVector3 operator*(const double scalar, const FGColumnVector3& A);
  friend FGColumnVector3 operator*(const FGMatrix33& M, const FGColumnVector3& V);

  friend ostream& operator<<(ostream& os, const FGColumnVector3& col);

  double& operator()(int m) const;

  FGColumnVector3 multElementWise(const FGColumnVector3& V);

private:
  double *data;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
