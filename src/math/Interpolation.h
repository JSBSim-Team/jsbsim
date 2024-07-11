#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <vector>
#include "FGMatrix.h"

double interpolate(const std::vector<double>& queryPoint, const PointCloud& points);

#endif // INTERPOLATION_H