#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <vector>
#include "FGMatrix.h"
#include "VectorHash.h"

double interpolate(const std::vector<double>& queryPoint, const PointCloud& points);

// Function prototypes
double findLowerBound(const std::vector<double>& vec, double value);
double getValueAtPoint(const PointCloud& points, const std::vector<double>& queryCoords);
double clamp(double value, double min, double max);

// Constants
const double EPSILON = std::numeric_limits<double>::epsilon();

#endif // INTERPOLATION_H