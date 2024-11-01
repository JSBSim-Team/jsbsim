#include "Interpolation.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

// Logging macro
#define LOG(level, message)                                                    \
  std::cout << "[" << level << "] " << __FILE__ << ":" << __LINE__ << " - "    \
            << message << std::endl

// Utility function to convert vector to string for logging
std::string vectorToString(const std::vector<double> &vec) {
  std::ostringstream oss;
  oss << std::setprecision(6) << std::fixed;
  oss << "(";
  for (size_t i = 0; i < vec.size(); ++i) {
    oss << vec[i];
    if (i < vec.size() - 1)
      oss << ", ";
  }
  oss << ")";
  return oss.str();
}

// Function to get the value at a specific point in the point cloud
double getValueAtPoint(const PointCloud &points,
                       const std::vector<double> &queryCoords) {

  // Adjust query coordinates values within epsilon to zero
  std::vector<double> adjustedQueryCoords = queryCoords;
  for (auto &value : adjustedQueryCoords) {
    if (std::abs(value) < EPSILON) {
      value = 0.0;
    }
  }

  auto it = points.pointMap.find(adjustedQueryCoords);
  if (it != points.pointMap.end()) {
    return it->second;
  }

  // Prepare error message with query point details
  std::ostringstream errorMsg;
  errorMsg << "Value not found for query point: "
           << vectorToString(queryCoords);

  LOG("ERROR", errorMsg.str());

  // Throw exception with detailed message
  throw std::runtime_error(errorMsg.str());
}

// Function to clamp a value between a minimum and maximum
double clamp(double value, double min, double max) {
  return std::max(min, std::min(value, max));
}

// Helper function for multi-linear interpolation
double interpolateRecursive(
    const std::vector<double> &queryPoint, const PointCloud &points,
    const std::vector<std::vector<double>::const_iterator> &lowerBounds,
    const std::vector<std::vector<double>::const_iterator> &upperBounds,
    size_t dimension) {
  if (dimension == points.numDimensions) {
    std::vector<double> point(points.numDimensions);
    for (size_t i = 0; i < points.numDimensions; ++i) {
      point[i] = *lowerBounds[i];
    }
    return getValueAtPoint(points, point);
  }

  double lowerValue = *lowerBounds[dimension];
  double upperValue = *upperBounds[dimension];

  // Handle the case where lower and upper bounds are the same
  if (std::abs(upperValue - lowerValue) < EPSILON) {
    std::vector<std::vector<double>::const_iterator> nextBounds = lowerBounds;
    return interpolateRecursive(queryPoint, points, nextBounds, nextBounds,
                                dimension + 1);
  }

  double t = (queryPoint[dimension] - lowerValue) / (upperValue - lowerValue);

  std::vector<std::vector<double>::const_iterator> nextLowerBounds =
      lowerBounds;
  std::vector<std::vector<double>::const_iterator> nextUpperBounds =
      upperBounds;

  nextUpperBounds[dimension] = lowerBounds[dimension];
  double v1 = interpolateRecursive(queryPoint, points, nextLowerBounds,
                                   nextUpperBounds, dimension + 1);

  nextLowerBounds[dimension] = upperBounds[dimension];
  double v2 = interpolateRecursive(queryPoint, points, nextLowerBounds,
                                   nextUpperBounds, dimension + 1);

  // TODO: Handle cases where we clamp one and not the other
  // Distance should be based on the clamped value, not the true query point
  return v1 * (1 - t) + v2 * t;
}

double interpolate(const std::vector<double> &queryPoint,
                   const PointCloud &points) {
  if (queryPoint.size() != points.numDimensions) {
    throw std::runtime_error(
        "Query point dimension does not match PointCloud dimension");
  }

  std::vector<double> clampedQueryPoint = queryPoint;
  std::vector<std::vector<double>::const_iterator> lowerBounds(
      points.numDimensions);
  std::vector<std::vector<double>::const_iterator> upperBounds(
      points.numDimensions);

  for (size_t i = 0; i < points.numDimensions; ++i) {
    double clampMax =
        std::max(points.uniqueValues[i].front(), points.uniqueValues[i].back());
    double clampMin =
        std::min(points.uniqueValues[i].front(), points.uniqueValues[i].back());

    clampedQueryPoint[i] = clamp(queryPoint[i], clampMin, clampMax);

    lowerBounds[i] =
        std::lower_bound(points.uniqueValues[i].begin(),
                         points.uniqueValues[i].end(), clampedQueryPoint[i]);
    if (lowerBounds[i] != points.uniqueValues[i].begin() &&
        *lowerBounds[i] > clampedQueryPoint[i]) {
      --lowerBounds[i];
    }

    upperBounds[i] = std::next(lowerBounds[i]);
    if (upperBounds[i] == points.uniqueValues[i].end()) {

      upperBounds[i] = lowerBounds[i];
    }
  }

  return interpolateRecursive(clampedQueryPoint, points, lowerBounds,
                              upperBounds, 0);
}
