#include "Interpolation.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <set>
#include <unordered_map>
#include <sstream>

double findLowerBound(const std::vector<double>& vec, double value) {
    auto it = std::lower_bound(vec.begin(), vec.end(), value);
    if (it == vec.end() || (it != vec.begin() && *it > value)) {
        --it;
    }
    return *it;
}

double getValueAtPoint(const PointCloud& points, const std::vector<double>& queryCoords) {
    auto it = points.pointMap.find(queryCoords);
    if (it != points.pointMap.end()) {
        return it->second;
    }
    
    // Prepare error message with query point details
    std::ostringstream errorMsg;
    errorMsg << "Value not found for query point: (";
    for (size_t i = 0; i < queryCoords.size(); ++i) {
        errorMsg << queryCoords[i];
        if (i < queryCoords.size() - 1) errorMsg << ", ";
    }
    errorMsg << ")";
    
    // Log error details
    std::cerr << "Error in getValueAtPoint: " << errorMsg.str() << std::endl;
    
    // Throw exception with detailed message
    throw std::runtime_error(errorMsg.str());
}

double interpolateRecursive(const std::vector<double>& queryPoint, const PointCloud& points, size_t dim) {
    if (dim == 0) {
        return getValueAtPoint(points, queryPoint);
    }

    double lower = findLowerBound(points.uniqueValues[dim - 1], queryPoint[dim - 1]);
    double upper = *std::upper_bound(points.uniqueValues[dim - 1].begin(), points.uniqueValues[dim - 1].end(), lower);

    std::vector<double> lowerCoords = queryPoint;
    std::vector<double> upperCoords = queryPoint;
    lowerCoords[dim - 1] = lower;
    upperCoords[dim - 1] = upper;

    double lowerValue = interpolateRecursive(lowerCoords, points, dim - 1);
    double upperValue = interpolateRecursive(upperCoords, points, dim - 1);

    return (upper - queryPoint[dim - 1]) / (upper - lower) * lowerValue + (queryPoint[dim - 1] - lower) / (upper - lower) * upperValue;
}

double interpolate(const std::vector<double>& queryPoint, const PointCloud& points) {
    return interpolateRecursive(queryPoint, points, points.numDimensions);
}