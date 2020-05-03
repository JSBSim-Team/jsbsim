#include <math/FGColumnVector3.h>
#include <math/FGMatrix33.h>

void assertVectorEqual(const char* _file, int _line,
                       const JSBSim::FGColumnVector3& x,
                       const JSBSim::FGColumnVector3& y,
                       double delta) {
  _TS_ASSERT_DELTA(_file, _line, x(1), y(1), delta);
  _TS_ASSERT_DELTA(_file, _line, x(2), y(2), delta);
  _TS_ASSERT_DELTA(_file, _line, x(3), y(3), delta);
}

void assertMatrixEqual(const char* _file, int _line,
                       const JSBSim::FGMatrix33& x,
                       const JSBSim::FGMatrix33& y,
                       double delta) {
  _TS_ASSERT_DELTA(_file, _line, x(1,1), y(1,1), delta);
  _TS_ASSERT_DELTA(_file, _line, x(1,2), y(1,2), delta);
  _TS_ASSERT_DELTA(_file, _line, x(1,3), y(1,3), delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,1), y(2,1), delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,2), y(2,2), delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,3), y(2,3), delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,1), y(3,1), delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,2), y(3,2), delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,3), y(3,3), delta);
}

void assertMatrixIsIdentity(const char* _file, int _line,
                            const JSBSim::FGMatrix33& x,
                            double delta) {
  _TS_ASSERT_DELTA(_file, _line, x(1,1), 1.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(1,2), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(1,3), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,1), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,2), 1.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(2,3), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,1), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,2), 0.0, delta);
  _TS_ASSERT_DELTA(_file, _line, x(3,3), 1.0, delta);
}

#define TS_ASSERT_VECTOR_EQUALS(x, y) assertVectorEqual(__FILE__, __LINE__, x, y, epsilon)
#define TS_ASSERT_MATRIX_EQUALS(x, y) assertMatrixEqual(__FILE__, __LINE__, x, y, epsilon)
#define TS_ASSERT_MATRIX_IS_IDENTITY(x) assertMatrixIsIdentity(__FILE__, __LINE__, x, epsilon)
