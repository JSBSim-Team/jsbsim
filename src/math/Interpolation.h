#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include "FGMatrix.h"
#include "VectorHash.h"
#include <vector>

double interpolate(const std::vector<double> &queryPoint,
                   const PointCloud &points);

// Function prototypes
double getValueAtPoint(const PointCloud &points,
                       const std::vector<double> &queryCoords);
double clamp(double value, double min, double max);

// Constants
const double EPSILON = std::numeric_limits<double>::epsilon();

#endif // INTERPOLATION_H