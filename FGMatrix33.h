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

#include "FGColumnVector3.h"


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DEFINITIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#define ID_MATRIX "$Id: FGMatrix33.h,v 1.1 2001/07/23 12:46:03 apeden Exp $"

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
FORWARD DECLARATIONS
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGColumnVector33;

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: MatrixException
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

// class MatrixException /* :  public exception */
// {
// public:
//   string Message;
// };

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
DECLARATION: FGMatrix33
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

class FGMatrix33
{
public:
  FGMatrix33(void);
  FGMatrix33(const FGMatrix33& A);
  ~FGMatrix33(void);

  FGMatrix33& operator=(const FGMatrix33& A);
  inline double& operator()(unsigned int row, unsigned int col) const {return data[row][col];}

  FGColumnVector3 operator*(const FGColumnVector3& Col);

  inline unsigned int Rows(void) const { return 3; }
  inline unsigned int Cols(void) const { return 3; }

  void T(void);
  void InitMatrix(void);
  void InitMatrix(double value);

  FGMatrix33 operator-(const FGMatrix33& B);
  FGMatrix33 operator+(const FGMatrix33& B);
  FGMatrix33 operator*(const FGMatrix33& B);
  FGMatrix33 operator/(const double scalar);
  FGMatrix33& operator<<(const float ff);

  friend ostream& operator<<(ostream& os, const FGMatrix33& M);
  friend istream& operator>>(istream& is, FGMatrix33& M);

  void operator-=(const FGMatrix33 &B);
  void operator+=(const FGMatrix33 &B);
  void operator*=(const FGMatrix33 &B);
  void operator*=(const double scalar);
  void operator/=(const double scalar);

  friend FGMatrix33 operator*(double scalar,FGMatrix33& A);

  void SetOParams(char delim,int width,int prec, int origin=0);

protected:
  double **data;

private:
  char delim;
  int width,prec,origin;
  void TransposeSquare(void);
  unsigned int rowCtr, colCtr;
  void Debug(void);
};

#endif
