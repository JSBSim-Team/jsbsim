#include "Interpolation.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <set>
#include <unordered_map>
#include <sstream>
#include <cxxabi.h>

// Constants
const int MAX_CALLSTACK_DEPTH = 128;
const double EPSILON = 1e-10;


// Function to find the lower bound in a sorted vector
double findLowerBound(const std::vector<double>& vec, double value) {
    auto it = std::lower_bound(vec.begin(), vec.end(), value);
    if (it == vec.end() || (it != vec.begin() && *it > value)) {
        --it;
    }
    return *it;
}

// Function to get the value at a specific point in the point cloud
double getValueAtPoint(const PointCloud& points, const std::vector<double>& queryCoords) {
    // Adjust query coordinates values within epsilon to zero
    std::vector<double> adjustedQueryCoords = queryCoords;
    for (auto& value : adjustedQueryCoords) {
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
        if (i < queryCoords.size() - 1) errorMsg << ", ";
    }
    errorMsg << ")";
    
    // Log error details with stack trace
    std::cerr << "Error in getValueAtPoint: " << errorMsg.str() << std::endl;
    
    // Throw exception with detailed message
    throw std::runtime_error(errorMsg.str());
}

// Recursive function to perform interpolation
double interpolateRecursive(const std::vector<double>& queryPoint, const PointCloud& points, size_t dim) {
    try {
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
    } catch (const std::exception& e) {
        // Log the parameters if an error has occurred
        std::cerr << "interpolateRecursive called with queryPoint: (";
        for (size_t i = 0; i < queryPoint.size(); ++i) {
            std::cerr << queryPoint[i];
            if (i < queryPoint.size() - 1) std::cerr << ", ";
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
double interpolate(const std::vector<double>& queryPoint, const PointCloud& points) {
    std::vector<double> clampedQueryPoint = queryPoint;
    
    // Clamp the query point coordinates to the valid range
    for (size_t i = 0; i < points.numDimensions; ++i) {
        clampedQueryPoint[i] = clamp(queryPoint[i], 
                                     points.uniqueValues[i].front(), 
                                     points.uniqueValues[i].back());
    }
    
    return interpolateRecursive(clampedQueryPoint, points, points.numDimensions);
}