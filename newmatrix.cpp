#ifdef __BORLANDC__
#include <condefs.h>
#endif

#include "FGMatrix.h"

//---------------------------------------------------------------------------
#ifdef __BORLANDC__
  USEUNIT("FGMatrix.cpp");
  #pragma argsused
#endif


int main(int argc, char* argv[])
{
  FGColumnVector mycol;
  FGMatrix       T(3,3);
  FGColumnVector result;
  FGMatrix       resultM(3,3);
//  FGMatrix result(3,1);

  mycol(1) = 0.50;
  mycol(2) = 2.00;
  mycol(3) = 4.00;

  T(1,1) = 1.0;
  T(1,2) = 0.0;
  T(1,3) = 0.0;
  T(2,1) = 0.0;
  T(2,2) = 1.0;
  T(2,3) = 0.0;
  T(3,1) = 0.0;
  T(3,2) = 0.0;
  T(3,3) = 1.0;

  try {
    result = (T*mycol + 2.5*mycol)*2.0;
  } catch (MatrixException mE) {
    cerr << ("A matrix exception was thrown "  +  mE.Message) << endl;
    exit(-1);
  }

  cout << result(1) << endl;
  cout << result(2) << endl;
  cout << result(3) << endl;

  return 0;
}
 