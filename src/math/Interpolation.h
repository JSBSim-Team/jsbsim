#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <vector>
#include <unordered_map>
#include "FGMatrix.h"

// Specialization of std::hash for std::vector<double>
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

double interpolate(const std::vector<double>& queryPoint, const PointCloud& points);

#endif // INTERPOLATION_H