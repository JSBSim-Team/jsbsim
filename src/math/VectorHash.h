#ifndef VECTOR_HASH_H
#define VECTOR_HASH_H

#include <vector>
#include <functional>

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

#endif // VECTOR_HASH_H