#ifndef VECTOR_HASH_H
#define VECTOR_HASH_H

#include <vector>
#include <functional>

// A variant of hash combine optimized for speed and low collisions.
namespace std {
    template<>
    struct hash<vector<double>> {
        size_t operator()(const vector<double>& v) const {
            constexpr size_t prime1 = 0x100000001b3;  // FNV-1a 64-bit prime
            constexpr size_t prime2 = 0x9e3779b9;     // Golden ratio prime

            size_t seed = 0xcbf29ce484222325;  // FNV-1a 64-bit offset basis
            seed ^= v.size() * prime1;  // Incorperate size

            for (const auto& i : v) {
                seed ^= hash<double>()(i) + prime2 + (seed << 6) + (seed >> 2);
            }

            return seed;
        }
    };
}

#endif // VECTOR_HASH_H