#pragma once

#include <array>
#include <cmath>
#include <random>
#include <algorithm>
#include <numeric>
#include <vector>

namespace vkengine {

class PerlinNoise {
private:
    std::vector<int> p;

public:
    // Constructor with default seed
    PerlinNoise();
    
    // Constructor with specified seed
    PerlinNoise(unsigned int seed);
    
    // Get noise value at coordinates (x, y, z)
    double noise(double x, double y, double z = 0.0) const;
    
    // Get noise value with octaves for more natural looking terrain
    double octaveNoise(double x, double y, int octaves, double persistence);

private:
    double fade(double t) const;
    double lerp(double t, double a, double b) const;
    double grad(int hash, double x, double y, double z) const;
};

} // namespace vkengine
