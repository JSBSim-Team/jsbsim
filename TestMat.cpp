#include "FGMatrix.h"


main()
{
  FGColumnVector forces;
  FGMatrix       T(3,3);
  FGColumnVector result;
//  FGMatrix result(3,1);

  forces(1) = 0.50;
  forces(2) = 2.00;
  forces(3) = 4.00;

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
    result = T*forces + forces;
  } catch (...) {
    cerr << "A matrix exception was thrown" << endl << endl;
    exit(-1);
  }

  cout << result(1) << endl;
  cout << result(2) << endl;
  cout << result(3) << endl;
}
