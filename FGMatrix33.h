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

#ifndef FGMATRIX33_H
#define FGMATRIX33_H

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

#include "FGColumnVector3.h"
#include "FGJSBBase.h"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MATRIX33 "$Id: FGMatrix33.h,v 1.12 2003/06/03 09:53:46 ehofman Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

namespace JSBSim {

class FGColumnVector3;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: MatrixException
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class MatrixException : public FGJSBBase
{
public:
  string Message;
};

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix33
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix33 : public FGJSBBase
{
public:
  FGMatrix33(void);
  FGMatrix33(int r, int c);
  FGMatrix33(const FGMatrix33& A);
  ~FGMatrix33(void);

  FGMatrix33& operator=(const FGMatrix33& A);
  inline double operator()(unsigned int row, unsigned int col) const {return data[row][col];}
  inline double& operator()(unsigned int row, unsigned int col) {return data[row][col];}

  FGColumnVector3 operator*(const FGColumnVector3& Col);

  inline unsigned int Rows(void) const { return 3; }
  inline unsigned int Cols(void) const { return 3; }

  void T(void);
  void InitMatrix(void);
  void InitMatrix(double value);
  
  //friend FGMatrix33 operator*(double scalar,FGMatrix33& A);

  FGMatrix33 operator-(const FGMatrix33& B);
  FGMatrix33 operator+(const FGMatrix33& B);
  FGMatrix33 operator*(const FGMatrix33& B);
  FGMatrix33 operator*(const double scalar);
  FGMatrix33 operator/(const double scalar);
  FGMatrix33& operator<<(const double ff);

  friend ostream& operator<<(ostream& os, const FGMatrix33& M);
  friend istream& operator>>(istream& is, FGMatrix33& M);

  void operator-=(const FGMatrix33 &B);
  void operator+=(const FGMatrix33 &B);
  void operator*=(const FGMatrix33 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

protected:
  double data[4][4];

private:
  void TransposeSquare(void);
  unsigned int rowCtr, colCtr;
  void Debug(int from);
};
}
#endif
