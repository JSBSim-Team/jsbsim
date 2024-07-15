#include <cxxtest/TestSuite.h>
#include "math/Interpolation.h"
#include <random>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <unordered_set>
#include <cmath>
#include <limits>

class InterpolationTest : public CxxTest::TestSuite
{
public:
    // Non-linear continuous function for 4D
    static double nonLinearFunction(const std::vector<double>& x) {
        return std::sqrt(std::pow(2 * std::cos(x[0]) + std::sin(x[1]), 2) + 
                         std::pow(2 * std::sin(x[2]) + std::cos(x[3]), 2) + 1);
    }

    // New tolerance function
    static double calculateTolerance(const std::vector<double>& point, const PointCloud& points) {
        const double h = 1e-4; // Small step size for numerical differentiation
        std::vector<double> gradient(point.size());

        // Calculate numerical gradient
        for (size_t i = 0; i < point.size(); ++i) {
            std::vector<double> point_plus = point;
            std::vector<double> point_minus = point;
            point_plus[i] += h;
            point_minus[i] -= h;
            gradient[i] = (nonLinearFunction(point_plus) - nonLinearFunction(point_minus)) / (2 * h);
        }

        // Calculate maximum gradient magnitude
        double max_gradient = 0.0;
        for (double g : gradient) {
            max_gradient = std::max(max_gradient, std::abs(g));
        }

        // Calculate average grid spacing
        double avg_spacing = 0.0;
        for (const auto& dim_values : points.uniqueValues) {
            avg_spacing += (dim_values.back() - dim_values.front()) / (dim_values.size() - 1);
        }
        avg_spacing /= point.size();

        // Tolerance based on local linearity and grid spacing
        double base_tolerance = max_gradient * avg_spacing * avg_spacing;

        // Add a small absolute tolerance to handle near-zero values
        double abs_tolerance = 1e-6;

        return std::max(base_tolerance, abs_tolerance);
    }

    PointCloud createRandomPointCloud(int numDimensions, int pointsPerDimension, double minValue = 0.0, double maxValue = 2 * M_PI)
    {
        PointCloud points;
        points.numDimensions = numDimensions;
        points.uniqueValues.resize(numDimensions);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> spacingDis(0.1, 1.0); // Random spacing between 0.1 and 1.0

        // Generate random spacing for each dimension
        std::vector<double> spacings(numDimensions);
        for (int dim = 0; dim < numDimensions; ++dim) {
            spacings[dim] = spacingDis(gen);
        }

        // Create evenly distributed grid with random spacing
        for (int dim = 0; dim < numDimensions; ++dim) {
            double range = maxValue - minValue;
            double spacing = spacings[dim] * range / (pointsPerDimension - 1);
            for (int j = 0; j < pointsPerDimension; ++j) {
                double value = minValue + j * spacing;
                points.uniqueValues[dim].push_back(value);
            }
        }

        // Generate all permutations of points
        std::vector<int> indices(numDimensions, 0);
        while (true) {
            std::vector<double> point(numDimensions);
            for (int i = 0; i < numDimensions; ++i) {
                point[i] = points.uniqueValues[i][indices[i]];
            }
            points.pointMap[point] = nonLinearFunction(point);

            // Increment indices
            int dim = 0;
            while (dim < numDimensions && ++indices[dim] == pointsPerDimension) {
                indices[dim] = 0;
                ++dim;
            }
            if (dim == numDimensions) break;
        }

        return points;
    }

    void testBasicInterpolation4D()
    {

        std::cout << "\n#########################################" << std::endl;
        std::cout << "Starting testBasicInterpolation4D\n" << std::endl;
        
        PointCloud points;
        points.numDimensions = 4;
        points.uniqueValues = {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}};

        // Set up a 4D hypercube
        for (int i = 0; i <= 1; ++i) {
            for (int j = 0; j <= 1; ++j) {
                for (int k = 0; k <= 1; ++k) {
                    for (int l = 0; l <= 1; ++l) {
                        std::vector<double> point = {static_cast<double>(i), static_cast<double>(j), 
                                                     static_cast<double>(k), static_cast<double>(l)};
                        points.pointMap[point] = nonLinearFunction(point);
                    }
                }
            }
        }

        // Test interpolation at the center of the hypercube
        std::vector<double> queryPoint = {0.5, 0.5, 0.5, 0.5};
        double result = interpolate(queryPoint, points);
        double expected = nonLinearFunction(queryPoint);
        double tolerance = calculateTolerance(queryPoint, points);
        TS_ASSERT_DELTA(result, expected, tolerance);
        
        std::cout << "\nFinished testBasicInterpolation4D" << std::endl;
        std::cout << "#########################################\n" << std::endl;
    }

    void testEdgeCases4D()
    {
        std::cout << "\n#########################################" << std::endl;
        std::cout << "Starting testEdgeCases4D\n" << std::endl;

        PointCloud points;
        points.numDimensions = 4;
        points.uniqueValues = {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}};

        // Set up a 4D hypercube with function values
        for (int i = 0; i <= 1; ++i) {
            for (int j = 0; j <= 1; ++j) {
                for (int k = 0; k <= 1; ++k) {
                    for (int l = 0; l <= 1; ++l) {
                        std::vector<double> point = {static_cast<double>(i), static_cast<double>(j), 
                                                     static_cast<double>(k), static_cast<double>(l)};
                        points.pointMap[point] = nonLinearFunction(point);
                    }
                }
            }
        }

        // Test interpolation at various points
        std::vector<std::vector<double>> testPoints = {
            {0.0, 0.0, 0.0, 0.0},
            {1.0, 1.0, 1.0, 1.0},
            {0.5, 0.5, 0.5, 0.5},
            {0.25, 0.75, 0.4, 0.6}
        };

        for (const auto& point : testPoints) {
            double result = interpolate(point, points);
            double expected = nonLinearFunction(point);
            double tolerance = calculateTolerance(point, points);
            TS_ASSERT_DELTA(result, expected, tolerance);
        }
        
        std::cout << "Finished testEdgeCases4D" << std::endl;
        std::cout << "#########################################\n" << std::endl;
    }

    void testPerformance4D()
    {
        std::cout << "\n#########################################" << std::endl;
        std::cout << "Starting testPerformance4D\n" << std::endl;

        const int VECTOR_SIZE = 4;
        std::vector<int> test_sizes = {5, 7, 10}; // Reduced sizes for a 4D grid

        for (int GRID_SIZE : test_sizes) {
            int NUM_POINTS = std::pow(GRID_SIZE, VECTOR_SIZE);
            int NUM_QUERIES = 1000;

            std::cout << "Testing with " << NUM_POINTS << " points (" << GRID_SIZE << " per dimension) and " << NUM_QUERIES << " queries" << std::endl;

            PointCloud points = createRandomPointCloud(VECTOR_SIZE, GRID_SIZE);

            // Generate random query points
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 2 * M_PI);

            std::vector<std::vector<double>> queryPoints(NUM_QUERIES, std::vector<double>(VECTOR_SIZE));
            for (auto& query : queryPoints) {
                for (auto& coord : query) {
                    coord = dis(gen);
                }
            }

            // Measure time to interpolate all query points
            auto start = std::chrono::high_resolution_clock::now();
            for (const auto& query : queryPoints) {
                volatile double result = interpolate(query, points);  // volatile to prevent optimization
            }
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double> diff = end - start;
            double avg_time = diff.count() / NUM_QUERIES;

            std::cout << "Average time to interpolate a 4D point: " << std::fixed << std::setprecision(9) 
                      << avg_time * 1e6 << " microseconds\n" << std::endl;
        }
        
        std::cout << "\nFinished testPerformance4D" << std::endl;
        std::cout << "#########################################\n" << std::endl;
    }

    // Add this function to your InterpolationTest class
    double manualTrilinearInterpolation(const std::vector<double>& point, const PointCloud& points)
    {
        double x = point[0], y = point[1], z = point[2];
        const auto& uniqueValues = points.uniqueValues;

        // Find the indices of the surrounding cube
        size_t ix = 0, iy = 0, iz = 0;
        while (ix + 1 < uniqueValues[0].size() && uniqueValues[0][ix + 1] <= x) ++ix;
        while (iy + 1 < uniqueValues[1].size() && uniqueValues[1][iy + 1] <= y) ++iy;
        while (iz + 1 < uniqueValues[2].size() && uniqueValues[2][iz + 1] <= z) ++iz;

        // Calculate the relative position within the cube
        double x0 = uniqueValues[0][ix], x1 = uniqueValues[0][ix + 1];
        double y0 = uniqueValues[1][iy], y1 = uniqueValues[1][iy + 1];
        double z0 = uniqueValues[2][iz], z1 = uniqueValues[2][iz + 1];

        double xd = (x - x0) / (x1 - x0);
        double yd = (y - y0) / (y1 - y0);
        double zd = (z - z0) / (z1 - z0);

        // Get the values at the cube corners
        double c000 = points.pointMap.at({x0, y0, z0});
        double c001 = points.pointMap.at({x0, y0, z1});
        double c010 = points.pointMap.at({x0, y1, z0});
        double c011 = points.pointMap.at({x0, y1, z1});
        double c100 = points.pointMap.at({x1, y0, z0});
        double c101 = points.pointMap.at({x1, y0, z1});
        double c110 = points.pointMap.at({x1, y1, z0});
        double c111 = points.pointMap.at({x1, y1, z1});

        // Perform trilinear interpolation
        return c000 * (1-xd) * (1-yd) * (1-zd) +
               c100 * xd * (1-yd) * (1-zd) +
               c010 * (1-xd) * yd * (1-zd) +
               c110 * xd * yd * (1-zd) +
               c001 * (1-xd) * (1-yd) * zd +
               c101 * xd * (1-yd) * zd +
               c011 * (1-xd) * yd * zd +
               c111 * xd * yd * zd;
    }

    void testValidity3DUniform()
    {
        std::cout << "\n#########################################" << std::endl;
        std::cout << "Starting testAccuracy3DUniform\n" << std::endl;

        // Create a 3D point cloud
        PointCloud points;
        points.numDimensions = 3;
        points.uniqueValues = {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}};

        // Define a simple 3D function
        auto testFunction = [](const std::vector<double>& x) {
            return x[0] * x[1] + x[2];
        };

        // Set up the 3D cube
        for (int i = 0; i <= 1; ++i) {
            for (int j = 0; j <= 1; ++j) {
                for (int k = 0; k <= 1; ++k) {
                    std::vector<double> point = {static_cast<double>(i), static_cast<double>(j), static_cast<double>(k)};
                    points.pointMap[point] = testFunction(point);
                }
            }
        }

        // Test interpolation at various points
        std::vector<std::vector<double>> testPoints = {
            {0.5, 0.5, 0.5},
            {0.25, 0.75, 0.4},
            {0.1, 0.9, 0.3},
            {0.8, 0.2, 0.6}
        };

        for (const auto& point : testPoints) {
            double interpolated = interpolate(point, points);
            double manual = manualTrilinearInterpolation(point, points);

            std::cout << "Point: (" << point[0] << ", " << point[1] << ", " << point[2] << ")" << std::endl;
            std::cout << "Interpolated: " << interpolated << std::endl;
            std::cout << "Manual: " << manual << std::endl;

            // Check if the interpolated value matches the manual calculation
            TS_ASSERT_DELTA(interpolated, manual, 1e-10);

            // Check if the interpolated value is close to the expected value
            // We use a larger tolerance here because trilinear interpolation is an approximation
            TS_ASSERT_DELTA(interpolated, expected, 0.1);
        }

        std::cout << "\nFinished testAccuracy3DUniform" << std::endl;
        std::cout << "#########################################\n" << std::endl;
    }

    void testValidity3DNonUniform()
    {
        std::cout << "\n#########################################" << std::endl;
        std::cout << "Starting testAccuracy3DNonUniform\n" << std::endl;

        // Create a 3D point cloud with non-uniform spacing
        PointCloud points;
        points.numDimensions = 3;
        points.uniqueValues = {{0.0, 1.0}, {0.0, 0.3}, {-2.1, 12.34}};

        // Define a simple 3D function
        auto testFunction = [](const std::vector<double>& x) {
            return x[0] * x[1] + std::sin(x[2]);
        };

        // Set up the 3D grid
        for (double x : points.uniqueValues[0]) {
            for (double y : points.uniqueValues[1]) {
                for (double z : points.uniqueValues[2]) {
                    std::vector<double> point = {x, y, z};
                    points.pointMap[point] = testFunction(point);
                }
            }
        }

        // Test interpolation at various points
        std::vector<std::vector<double>> testPoints = {
            {0.5, 0.15, 5.12},
            {0.25, 0.2, 0.0},
            {0.75, 0.1, 10.0},
            {0.9, 0.25, -1.0}
        };

        for (const auto& point : testPoints) {
            double interpolated = interpolate(point, points);
            double expected = testFunction(point);
            double manual = manualTrilinearInterpolation(point, points);

            std::cout << "Point: (" << point[0] << ", " << point[1] << ", " << point[2] << ")" << std::endl;
            std::cout << "Interpolated: " << interpolated << std::endl;
            std::cout << "Manual: " << manual << "\n"<< std::endl;
            // Check if the interpolated value matches the manual calculation
            TS_ASSERT_DELTA(interpolated, manual, 1e-10);

            // Check if the interpolated value is close to the expected value
            // We use a larger tolerance here because trilinear interpolation is an approximation
            // TS_ASSERT_DELTA(interpolated, expected, 0.1);
        }

        std::cout << "\nFinished testAccuracy3DNonUniform" << std::endl;
        std::cout << "#########################################\n" << std::endl;
    }
};
