
#include <cstdint>
#include <cstddef>

// ——— Bit-interleaving to make a 3D Morton code ———
static inline uint64_t expandBits(uint32_t v) {
    uint64_t x = v;
    x = (x | (x << 16)) & 0x0000FFFF0000FFFFULL;
    x = (x | (x <<  8)) & 0x00FF00FF00FF00FFULL;
    x = (x | (x <<  4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = (x | (x <<  2)) & 0x3333333333333333ULL;
    x = (x | (x <<  1)) & 0x5555555555555555ULL;
    return x;
}

static inline uint64_t morton3D(uint32_t x, uint32_t y, uint32_t z) {
    // interleave bits: …z2y2x2 z1y1x1 z0y0x0
    return (expandBits(x) << 2)
         | (expandBits(y) << 1)
         |  expandBits(z);
}

// ——— SplitMix64 mixer for avalanche diffusion ———
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x  = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x  = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}