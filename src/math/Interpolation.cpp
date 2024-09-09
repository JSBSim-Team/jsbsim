#include "Interpolation.h"
#include <algorithm>
#include <cxxabi.h>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// Function to find the lower bound in a sorted vector
double findLowerBound(const std::vector<double> &vec, double value) {
  auto it = std::lower_bound(vec.begin(), vec.end(), value);
  if (it == vec.end() || (it != vec.begin() && *it > value)) {
    --it;
  }
  return *it;
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
  errorMsg << "Value not found for query point: (";
  for (size_t i = 0; i < queryCoords.size(); ++i) {
    errorMsg << queryCoords[i];
    if (i < queryCoords.size() - 1)
      errorMsg << ", ";
  }
  errorMsg << ")";

  // Log error details with stack trace
  std::cerr << "Error in getValueAtPoint: " << errorMsg.str() << std::endl;

  // Throw exception with detailed message
  throw std::runtime_error(errorMsg.str());
}

// Recursive function to perform interpolation
double interpolateRecursive(const std::vector<double> &queryPoint,
                            const PointCloud &points, size_t dim) {
  try {
    if (dim == 0) {
      return getValueAtPoint(points, queryPoint);
    }

    const auto &uniqueValues = points.uniqueValues[dim - 1];
    auto lowerIt = std::lower_bound(uniqueValues.begin(), uniqueValues.end(),
                                    queryPoint[dim - 1]);

    // Handle the case where the query point is outside the range
    if (lowerIt == uniqueValues.end()) {
      return interpolateRecursive(queryPoint, points, dim - 1);
    }

    double lower = *lowerIt;
    auto upperIt = std::upper_bound(lowerIt, uniqueValues.end(), lower);

    // If there's only one value or query point is exactly on a known point
    if (upperIt == uniqueValues.end() ||
        std::abs(queryPoint[dim - 1] - lower) < EPSILON) {
      std::vector<double> newQueryPoint = queryPoint;
      newQueryPoint[dim - 1] = lower;
      return interpolateRecursive(newQueryPoint, points, dim - 1);
    }

    double upper = *upperIt;

    std::vector<double> lowerCoords = queryPoint;
    std::vector<double> upperCoords = queryPoint;
    lowerCoords[dim - 1] = lower;
    upperCoords[dim - 1] = upper;

    double lowerValue, upperValue;

    try {
      lowerValue = interpolateRecursive(lowerCoords, points, dim - 1);
    } catch (const std::runtime_error &) {
      // If lower point doesn't exist, use upper point
      return interpolateRecursive(upperCoords, points, dim - 1);
    }

    try {
      upperValue = interpolateRecursive(upperCoords, points, dim - 1);
    } catch (const std::runtime_error &) {
      // If upper point doesn't exist, use lower point
      return lowerValue;
    }

    // Perform linear interpolation
    double t = (queryPoint[dim - 1] - lower) / (upper - lower);
    return (1 - t) * lowerValue + t * upperValue;
  } catch (const std::exception &e) {
    // Log the parameters if an error has occurred
    std::cerr << "interpolateRecursive called with queryPoint: (";
    for (size_t i = 0; i < queryPoint.size(); ++i) {
      std::cerr << queryPoint[i];
      if (i < queryPoint.size() - 1)
        std::cerr << ", ";
    }
    std::cerr << "), dim: " << dim << std::endl;
    // Rethrow the exception to propagate it up the call stack
    throw;
  }
}

// Function to clamp a value between a minimum and maximum
double clamp(double value, double min, double max) {
  return std::max(min, std::min(value, max));
}

// Function to perform interpolation
double interpolate(const std::vector<double> &queryPoint,
                   const PointCloud &points) {
  std::vector<double> clampedQueryPoint = queryPoint;

  // Clamp the query point coordinates to the valid range
  for (size_t i = 0; i < points.numDimensions; ++i) {
    clampedQueryPoint[i] = clamp(queryPoint[i], points.uniqueValues[i].front(),
                                 points.uniqueValues[i].back());
  }

  return interpolateRecursive(clampedQueryPoint, points, points.numDimensions);
}