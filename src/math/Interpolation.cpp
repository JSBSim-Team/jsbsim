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

// Function to generate all vertices of a hypercube
void generateHypercubeVertices(
    const std::vector<std::pair<double, double>> &bounds, size_t dim,
    std::vector<double> current, std::vector<std::vector<double>> &vertices) {
  if (dim == bounds.size()) {
    vertices.push_back(current);
    return;
  }

  current.push_back(bounds[dim].first);
  generateHypercubeVertices(bounds, dim + 1, current, vertices);
  current.back() = bounds[dim].second;
  generateHypercubeVertices(bounds, dim + 1, current, vertices);
}

// Function to calculate Euclidean distance between two points
double euclideanDistance(const std::vector<double> &a,
                         const std::vector<double> &b) {
  return std::sqrt(
      std::inner_product(a.begin(), a.end(), b.begin(), 0.0, std::plus<>(),
                         [](double x, double y) { return (x - y) * (x - y); }));
}

// Main interpolation function using linear interpolation with inverse distance
// weighting
double interpolate(const std::vector<double> &queryPoint,
                   const PointCloud &points) {

  std::vector<double> clampedQueryPoint = queryPoint;
  for (size_t i = 0; i < points.numDimensions; ++i) {
    clampedQueryPoint[i] = clamp(queryPoint[i], points.uniqueValues[i].front(),
                                 points.uniqueValues[i].back());
  }

  // Find the hypercube containing the query point
  std::vector<std::pair<double, double>> bounds(points.numDimensions);
  for (size_t i = 0; i < points.numDimensions; ++i) {
    bounds[i].first =
        findLowerBound(points.uniqueValues[i], clampedQueryPoint[i]);
    bounds[i].second =
        *std::upper_bound(points.uniqueValues[i].begin(),
                          points.uniqueValues[i].end(), bounds[i].first);
  }

  // Generate all vertices of the hypercube
  std::vector<std::vector<double>> vertices;
  generateHypercubeVertices(bounds, 0, std::vector<double>(), vertices);

  // Interpolate using inverse distance weighting
  double weightedSum = 0.0;
  double weightSum = 0.0;

  for (const auto &vertex : vertices) {
    double distance = euclideanDistance(clampedQueryPoint, vertex);
    if (distance < EPSILON) {
      double value = getValueAtPoint(points, vertex);
      return value;
    }
    double weight = 1.0 / distance;
    double value = getValueAtPoint(points, vertex);
    weightedSum += weight * value;
    weightSum += weight;
  }

  double result = weightedSum / weightSum;
  return result;
}
