namespace vkengine {
    
// Define block types
enum class BlockType {
    AIR,
    DIRT,
    GRASS,
    STONE,
    SAND,
    WATER,
    WOOD,
    LEAVES
    // Add more block types as needed
};

// Direction enum for block faces
enum class Direction {
    TOP,
    BOTTOM,
    FRONT,
    BACK,
    LEFT,
    RIGHT
};

enum ChunkFlags {
    NONE = 0,
    MESH_GENERATED = 1 << 0,
    DEFAULT_TERRAIN_GENERATED = 1 << 1,
    UP_TO_DATE = 1 << 2
};

enum ChunkManagerFlags {
    GENERATE_CHUNKS = 1 << 0,
};


enum class RenderMode {
    UV,
    WIREFRAME,
    TEXTURE,
    COLOR
};

}