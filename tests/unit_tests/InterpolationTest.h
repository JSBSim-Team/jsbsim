#include "math/Interpolation.h"
#include <chrono>
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_set>

class InterpolationTest : public CxxTest::TestSuite {
public:
  // Non-linear continuous function for multi-dimensional testing
  static double nonLinearFunction(const std::vector<double> &x) {
    double sum = 0;
    for (size_t i = 0; i < x.size(); ++i) {
      sum += std::sin(x[i]) * std::cos(x[(i + 1) % x.size()]);
    }
    return sum / x.size();
  }

  // Tolerance function
  static double calculateTolerance(const std::vector<double> &point,
                                   const PointCloud &points) {
    const double h = 1e-4; // Small step size for numerical differentiation
    std::vector<double> gradient(point.size());

    // Calculate numerical gradient
    for (size_t i = 0; i < point.size(); ++i) {
      std::vector<double> point_plus = point;
      std::vector<double> point_minus = point;
      point_plus[i] += h;
      point_minus[i] -= h;
      gradient[i] =
          (nonLinearFunction(point_plus) - nonLinearFunction(point_minus)) /
          (2 * h);
    }

    // Calculate maximum gradient magnitude
    double max_gradient = 0.0;
    for (double g : gradient) {
      max_gradient = std::max(max_gradient, std::abs(g));
    }

    // Calculate average grid spacing
    double avg_spacing = 0.0;
    for (const auto &dim_values : points.uniqueValues) {
      avg_spacing +=
          (dim_values.back() - dim_values.front()) / (dim_values.size() - 1);
    }
    avg_spacing /= point.size();

    // Tolerance based on local linearity and grid spacing
    double base_tolerance = max_gradient * avg_spacing * avg_spacing;

    // Add a small absolute tolerance to handle near-zero values
    double abs_tolerance = 1e-6;

    return std::max(base_tolerance, abs_tolerance);
  }

  PointCloud createRandomPointCloud(int numDimensions, int pointsPerDimension,
                                    double minValue = 0.0,
                                    double maxValue = 2 * M_PI) {
    PointCloud points;
    points.numDimensions = numDimensions;
    points.uniqueValues.resize(numDimensions);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> spacingDis(0.1, 1.0);

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
      if (dim == numDimensions)
        break;
    }

    return points;
  }

  void testBasicInterpolation() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testBasicInterpolation\n" << std::endl;

    PointCloud points = createRandomPointCloud(4, 2);

    // Test interpolation at the center of the hypercube
    std::vector<double> queryPoint(4, 0.5);
    double result = interpolate(queryPoint, points);
    double expected = nonLinearFunction(queryPoint);
    double tolerance = calculateTolerance(queryPoint, points);
    TS_ASSERT_DELTA(result, expected, tolerance);

    std::cout << "\nFinished testBasicInterpolation" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testEdgeCases() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testEdgeCases\n" << std::endl;

    PointCloud points = createRandomPointCloud(3, 2);

    std::vector<std::vector<double>> testPoints = {
        {0.0, 0.0, 0.0},
        {2 * M_PI, 2 * M_PI, 2 * M_PI},
        {M_PI, M_PI, M_PI},
        {M_PI / 2, 3 * M_PI / 4, M_PI / 4}};

    for (const auto &point : testPoints) {
      double result = interpolate(point, points);
      double expected = nonLinearFunction(point);
      double tolerance = calculateTolerance(point, points);
      TS_ASSERT_DELTA(result, expected, tolerance);
    }

    std::cout << "Finished testEdgeCases" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testPerformance() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testPerformance\n" << std::endl;

    const int VECTOR_SIZE = 4;
    std::vector<int> test_sizes = {3, 4, 5};

    for (int GRID_SIZE : test_sizes) {
      int NUM_POINTS = std::pow(GRID_SIZE, VECTOR_SIZE);
      int NUM_QUERIES = 1000;

      std::cout << "Testing with " << NUM_POINTS << " points (" << GRID_SIZE
                << " per dimension) and " << NUM_QUERIES << " queries"
                << std::endl;

      PointCloud points = createRandomPointCloud(VECTOR_SIZE, GRID_SIZE);

      // Generate random query points
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<> dis(0.0, 2 * M_PI);

      std::vector<std::vector<double>> queryPoints(
          NUM_QUERIES, std::vector<double>(VECTOR_SIZE));
      for (auto &query : queryPoints) {
        for (auto &coord : query) {
          coord = dis(gen);
        }
      }

      // Measure time to interpolate all query points
      auto start = std::chrono::high_resolution_clock::now();
      for (const auto &query : queryPoints) {
        volatile double result =
            interpolate(query, points); // volatile to prevent optimization
      }
      auto end = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> diff = end - start;
      double avg_time = diff.count() / NUM_QUERIES;

      std::cout << "Average time to interpolate a " << VECTOR_SIZE
                << "D point: " << std::fixed << std::setprecision(9)
                << avg_time * 1e6 << " microseconds\n"
                << std::endl;
    }

    std::cout << "\nFinished testPerformance" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testValidityUniform() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testValidityUniform\n" << std::endl;

    PointCloud points = createRandomPointCloud(3, 3);

    std::vector<std::vector<double>> testPoints = {
        {M_PI / 2, M_PI / 2, M_PI / 2},
        {M_PI / 4, 3 * M_PI / 4, M_PI / 2},
        {M_PI / 6, 5 * M_PI / 6, M_PI / 3},
        {4 * M_PI / 5, M_PI / 5, 3 * M_PI / 5}};

    for (const auto &point : testPoints) {
      double interpolated = interpolate(point, points);
      double expected = nonLinearFunction(point);

      std::cout << "Point: (" << point[0] << ", " << point[1] << ", "
                << point[2] << ")" << std::endl;
      std::cout << "Interpolated: " << interpolated << std::endl;
      std::cout << "Expected: " << expected << std::endl;

      double tolerance = calculateTolerance(point, points);
      TS_ASSERT_DELTA(interpolated, expected, tolerance);
    }

    std::cout << "\nFinished testValidityUniform" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testValidityNonUniform() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testValidityNonUniform\n" << std::endl;

    PointCloud points;
    points.numDimensions = 3;
    points.uniqueValues = {
        {0.0, 1.0, 2.5}, {0.0, 0.3, 1.2}, {-2.1, 0.0, 12.34}};

    // Set up the 3D grid
    for (double x : points.uniqueValues[0]) {
      for (double y : points.uniqueValues[1]) {
        for (double z : points.uniqueValues[2]) {
          std::vector<double> point = {x, y, z};
          points.pointMap[point] = nonLinearFunction(point);
        }
      }
    }

    std::vector<std::vector<double>> testPoints = {{0.5, 0.15, 5.12},
                                                   {1.75, 0.6, 0.0},
                                                   {2.0, 0.9, 10.0},
                                                   {0.9, 0.25, -1.0}};

    for (const auto &point : testPoints) {
      double interpolated = interpolate(point, points);
      double expected = nonLinearFunction(point);

      std::cout << "Point: (" << point[0] << ", " << point[1] << ", "
                << point[2] << ")" << std::endl;
      std::cout << "Interpolated: " << interpolated << std::endl;
      std::cout << "Expected: " << expected << "\n" << std::endl;

      double tolerance = calculateTolerance(point, points);
      TS_ASSERT_DELTA(interpolated, expected, tolerance);
    }

    std::cout << "\nFinished testValidityNonUniform" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testOutOfBoundsInterpolation() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testOutOfBoundsInterpolation\n" << std::endl;

    PointCloud points = createRandomPointCloud(3, 3);

    std::vector<std::vector<double>> testPoints = {
        {-1.0, M_PI, M_PI}, {3 * M_PI, M_PI, M_PI},
        {M_PI, -1.0, M_PI}, {M_PI, 3 * M_PI, M_PI},
        {M_PI, M_PI, -1.0}, {M_PI, M_PI, 3 * M_PI},
        {-1.0, -1.0, -1.0}, {3 * M_PI, 3 * M_PI, 3 * M_PI}};

    for (const auto &point : testPoints) {
      double outOfBoundsResult = interpolate(point, points);

      std::vector<double> inBoundsPoint = point;
      for (size_t i = 0; i < point.size(); ++i) {
        inBoundsPoint[i] =
            std::max(points.uniqueValues[i].front(),
                     std::min(point[i], points.uniqueValues[i].back()));
      }

      double inBoundsResult = interpolate(inBoundsPoint, points);

      std::cout << "Out-of-bounds point: (" << point[0] << ", " << point[1]
                << ", " << point[2] << ")" << std::endl;
      std::cout << "In-bounds point: (" << inBoundsPoint[0] << ", "
                << inBoundsPoint[1] << ", " << inBoundsPoint[2] << ")"
                << std::endl;
      std::cout << "Out-of-bounds result: " << outOfBoundsResult << std::endl;
      std::cout << "In-bounds result: " << inBoundsResult << std::endl;

      TS_ASSERT_DELTA(outOfBoundsResult, inBoundsResult, 1e-10);

      std::cout << std::endl;
    }

    std::cout << "Finished testOutOfBoundsInterpolation" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }

  void testClampingIssue() {
    std::cout << "\n#########################################" << std::endl;
    std::cout << "Starting testClampingIssue\n" << std::endl;

    // Create a 4D grid with more points per dimension
    PointCloud points;
    points.numDimensions = 4;
    points.uniqueValues = {
        {0.0, 0.5, 1.0},  // x-values
        {0.0, 0.5, 1.0},  // y-values
        {0.0, 0.5, 1.0},  // z-values
        {-1.0, -0.5, 0.0} // zz-values
    };

    // Define a non-linear function: f(x, y, z) = x * y + y * z + z * x
    auto nonLinearFunction = [](const std::vector<double> &x) {
      return x[0] * x[1] + x[1] * x[2] + x[2] * x[0] + x[3];
    };

    // Populate the point map with function values
    for (double x : points.uniqueValues[0]) {
      for (double y : points.uniqueValues[1]) {
        for (double z : points.uniqueValues[2]) {
          for (double zz : points.uniqueValues[3]) {
            std::vector<double> point = {x, y, z, zz};
            points.pointMap[point] = nonLinearFunction(point);
          }
        }
      }
    }

    // Choose a query point that requires interpolation
    std::vector<double> queryPoint = {0.25, 0.75, 0.5, -0.25};

    // Expected value calculated directly
    double expectedValue = nonLinearFunction(queryPoint);

    // Perform interpolation
    double interpolatedValue = interpolate(queryPoint, points);

    // Use a reasonable tolerance
    double tolerance = 1e-4;
    TS_ASSERT_DELTA(interpolatedValue, expectedValue, tolerance);

    std::cout << "\nFinished testClampingIssue" << std::endl;
    std::cout << "#########################################\n" << std::endl;
  }
};
