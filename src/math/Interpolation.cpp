#include "Interpolation.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <set>
#include <unordered_map>

namespace std {
    template<>
    struct hash<vector<double>> {
        size_t operator()(const vector<double>& v) const {
            size_t seed = v.size();
            for (const auto& i : v) {
                seed ^= hash<double>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}

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
    throw std::runtime_error("Value not found for given point");
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