#ifndef FGMATRIX_H
#define FGMATRIX_H

#include <vector>
#include <string>
#include <unordered_map>
#include "input_output/FGXMLElement.h"
#include "FGParameter.h"
#include "input_output/FGPropertyManager.h"
#include "VectorHash.h"

using namespace JSBSim;

struct PointND {
    std::vector<double> coordinates;
    double value;
    PointND(const std::vector<double>& coords, double val) : coordinates(coords), value(val) {}
};

struct PointCloud {
    std::vector<PointND> points;
    std::vector<std::vector<double>> uniqueValues; // Unique values for each dimension
    size_t numDimensions; // Number of dimensions
    std::unordered_map<std::vector<double>, double> pointMap; // Point map
};

class FGMatrix : public FGParameter {
public:
    FGMatrix(Element* el);
    double GetValue() const;
    std::string GetName() const;
    bool IsConstant() const;
    const std::vector<std::vector<double>>& GetMatrix() const;
    size_t GetNumDimensions() const;
    void Print() const;
    PointCloud pointCloud;
    static constexpr int PRINT_PRECISION = 8;

private:
    std::string name;
    size_t num_dimensions;
    std::vector<std::vector<double>> matrix;
    void populatePointCloud();  // Helper function to populate the PointCloud
};

#endif // FGMATRIX_H