#include <cxxtest/TestSuite.h>
#include "math/VectorHash.h"
#include <unordered_set>
#include <sstream>
#include <random> 
#include <chrono>

class VectorHashTest : public CxxTest::TestSuite
{
public:
    void testEmptyVector()
    {
        std::vector<double> v;
        std::hash<std::vector<double>> hasher;
        size_t hash_value = hasher(v);
        if (hash_value == 0) {
            std::cout << "Hash of empty vector is 0. This is unexpected. Hash value is: " << hash_value << std::endl;
        }
        TS_ASSERT(hash_value != 0);
    }

    void testSingleElementVector()
    {
        std::vector<double> v1{1.0};
        std::vector<double> v2{2.0};
        std::hash<std::vector<double>> hasher;
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v2));
    }

    void testMultiElementVector()
    {
        std::vector<double> v1{1.0, 2.0, 3.0};
        std::vector<double> v2{1.0, 2.0, 3.1};
        std::hash<std::vector<double>> hasher;
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v2));
    }

    void testOrderMatters()
    {
        std::vector<double> v1{1.5, 2.7, 3.9};
        std::vector<double> v2{3.9, 2.7, 1.5};
        std::vector<double> v3{1.5, 3.9, 2.7};
        std::vector<double> v4{2.7, 1.5, 3.9};
        std::vector<double> v5{-4.2, -5.8, -6.3};
        std::vector<double> v6{-6.3, -5.8, -4.2};
        std::vector<double> v7{0.0, 10.5, -7.2};
        std::vector<double> v8{-7.2, 10.5, 0.0};
        std::vector<double> v9{std::numeric_limits<double>::max(), std::numeric_limits<double>::min(), 0.0};
        std::vector<double> v10{0.0, std::numeric_limits<double>::min(), std::numeric_limits<double>::max()};

        std::hash<std::vector<double>> hasher;
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v2));
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v3));
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v4));
        TS_ASSERT_DIFFERS(hasher(v5), hasher(v6));
        TS_ASSERT_DIFFERS(hasher(v7), hasher(v8));
        TS_ASSERT_DIFFERS(hasher(v9), hasher(v10));
    }

    void testEdgeCases()
    {
        std::vector<double> v1{0.0, 0.0, 0.0};
        std::vector<double> v2{-0.0, 0.0, -0.0};
        std::vector<double> v3{std::numeric_limits<double>::epsilon(), 0.0, 0.0};
        std::vector<double> v4{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
        std::vector<double> v5{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};

        std::hash<std::vector<double>> hasher;
        TS_ASSERT_EQUALS(hasher(v1), hasher(v2));
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v3));
        TS_ASSERT_DIFFERS(hasher(v1), hasher(v4));
        TS_ASSERT_DIFFERS(hasher(v4), hasher(v5));
    }

    void testConsistency()
    {
        std::vector<double> v{1.0, 2.0, 3.0};
        std::hash<std::vector<double>> hasher;
        size_t hash1 = hasher(v);
        size_t hash2 = hasher(v);
        TS_ASSERT_EQUALS(hash1, hash2);
    }

    void testUnorderedSet()
    {
        std::unordered_set<std::vector<double>> set;
        set.insert({1.0, 2.0, 3.0});
        set.insert({4.0, 5.0, 6.0});
        TS_ASSERT_EQUALS(set.size(), 2);
        TS_ASSERT(set.find({1.0, 2.0, 3.0}) != set.end());
        TS_ASSERT(set.find({4.0, 5.0, 6.0}) != set.end());
        TS_ASSERT(set.find({1.0, 2.0, 3.1}) == set.end());
    }

    void testPerformance7D()
    {
        const int NUM_VECTORS = 1000000;
        const int VECTOR_SIZE = 7;
        std::vector<std::vector<double>> vectors(NUM_VECTORS, std::vector<double>(VECTOR_SIZE));
        std::hash<std::vector<double>> hasher;

        // Generate random vectors
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1000.0, 1000.0);
        for (auto& v : vectors) {
            for (auto& element : v) {
                element = dis(gen);
            }
        }

        // Measure time to hash all vectors
        auto start = std::chrono::high_resolution_clock::now();
        for (const auto& v : vectors) {
            volatile size_t hash = hasher(v);  // volatile to prevent optimization
        }
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> diff = end - start;
        double avg_time = diff.count() / NUM_VECTORS;

        std::cout << "Average time to hash a 7D vector: " << avg_time * 1e9 << " nanoseconds" << std::endl;
        
        // Assert that the average time is below a certain threshold (1 microsecond)
        TS_ASSERT(avg_time < 1e-6);
    }

    void testCollisions7D()
    {
        const int NUM_VECTORS = 1000000;
        const int VECTOR_SIZE = 7;
        std::unordered_set<size_t> hashes;
        std::hash<std::vector<double>> hasher;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(-1000.0, 1000.0);

        for (int i = 0; i < NUM_VECTORS; ++i) {
            std::vector<double> v(VECTOR_SIZE);
            for (auto& element : v) {
                element = dis(gen);
            }
            hashes.insert(hasher(v));
        }

        double collision_rate = 1.0 - (static_cast<double>(hashes.size()) / NUM_VECTORS);
        std::cout << "Collision rate for 7D vectors: " << collision_rate * 100 << "%" << std::endl;

        // Assert that the collision rate is below a certain threshold (e.g., 0.1%)
        TS_ASSERT(collision_rate < 0.001);
    }
};
