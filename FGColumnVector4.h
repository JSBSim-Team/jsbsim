/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Header: FGMatrix33.h
Author: Originally by Tony Peden [formatted and adapted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created
03/16/2000 JSB  Added exception throwing

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
#  if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
     SG_USING_STD(ostream);
     SG_USING_STD(istream);
     SG_USING_STD(cerr);
     SG_USING_STD(cout);
     SG_USING_STD(endl);
#  endif
#else
#  include <string>
#  if defined (sgi) && !defined(__GNUC__)
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

#define ID_COLUMNVECTOR4 "$Id: FGColumnVector4.h,v 1.5 2001/09/28 02:33:44 jberndt Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGColumnVector4
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector4 : public FGJSBBase
{
public:
  FGColumnVector4(void);
  FGColumnVector4(int m);
  FGColumnVector4(const FGColumnVector4& b);
  ~FGColumnVector4(void);
  
  FGColumnVector4 operator=(const FGColumnVector4& b);
  
  FGColumnVector4 operator*(const double scalar);
  //FGColumnVector4 operator*(const FGColumnVector4& V);   // Cross product operator
  FGColumnVector4 operator/(const double scalar);
  FGColumnVector4 operator+(const FGColumnVector4& B); // must not return reference
  FGColumnVector4 operator-(const FGColumnVector4& B);
  
  void operator-=(const FGColumnVector4 &B);
  void operator+=(const FGColumnVector4 &B);
  //void operator*=(const FGColumnVector4 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);
  
  inline double& operator()(int m) const { return data[m]; }
  
  FGColumnVector4& operator<<(const float ff);

  inline void InitMatrix(void) { data[1]=0; data[2]=0; data[3]=0; }
  inline void InitMatrix(float ff) { data[1]=ff; data[2]=ff; data[3]=ff; }

  float Magnitude(void);
  FGColumnVector4 Normalize(void);

  friend FGColumnVector4 operator*(const double scalar, const FGColumnVector4& A);

  friend ostream& operator<<(ostream& os, FGColumnVector4& col);


  FGColumnVector4 multElementWise(const FGColumnVector4& V);

private:
  double *data;
  int rowCtr;
  void Debug(void);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#endif
