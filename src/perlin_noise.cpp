#include "perlin_noise.hpp"

namespace vkengine {

// Constructor with default seed
PerlinNoise::PerlinNoise() {
    // Initialize with a fixed seed for consistent results
    PerlinNoise(42);
}

// Constructor with specified seed
PerlinNoise::PerlinNoise(unsigned int seed) {
    // Create a permutation array with values 0-255
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);
    
    // Shuffle the array with the seed
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);
    
    // Duplicate the permutation array to avoid overflow
    p.insert(p.end(), p.begin(), p.end());
}

double PerlinNoise::fade(double t) const {
    // Fade function as defined by Ken Perlin
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::lerp(double t, double a, double b) const {
    // Linear interpolation
    return a + t * (b - a);
}

double PerlinNoise::grad(int hash, double x, double y, double z) const {
    // Convert the lower 4 bits of the hash into 12 gradient directions
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double PerlinNoise::noise(double x, double y, double z) const {
    // Ensure we have a valid permutation table
    if (p.empty()) return 0.0;
    
    // Find the unit cube that contains the point
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    // Find relative x, y, z of point in cube
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    // Compute fade curves for each of x, y, z
    double u = fade(x);
    double v = fade(y);
    double w = fade(z);
    
    // Hash coordinates of the 8 cube corners
    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;
    
    // Add blended results from 8 corners of cube
    double result = lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                         grad(p[BA], x-1, y, z)),
                                 lerp(u, grad(p[AB], x, y-1, z),
                                         grad(p[BB], x-1, y-1, z))),
                        lerp(v, lerp(u, grad(p[AA+1], x, y, z-1),
                                         grad(p[BA+1], x-1, y, z-1)),
                                 lerp(u, grad(p[AB+1], x, y-1, z-1),
                                         grad(p[BB+1], x-1, y-1, z-1))));
                                         
    // Return result scaled to roughly the range [-1, 1]
    return result;
}

double PerlinNoise::octaveNoise(double x, double y, int octaves, double persistence) {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;  // Used for normalizing result to 0.0 - 1.0
    
    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        
        maxValue += amplitude;
        
        amplitude *= persistence;
        frequency *= 2.0;
    }
    
    // Normalize the result
    return total / maxValue;
}

} // namespace vkengine
