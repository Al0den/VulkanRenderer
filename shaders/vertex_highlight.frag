#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // gl_PointCoord provides coordinates within the point primitive,
    // ranging from (0,0) at one corner to (1,1) at the opposite.
    // We shift it so (0,0) is the center of the point.
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist_from_center = length(coord);

    // Define the radius of the point and its border.
    // A dist_from_center of 0.5 reaches the edge of the square point primitive.
    float point_radius = 0.5; 
    float border_thickness_relative = 0.2; // e.g., 20% of the radius is border

    float border_abs_thickness = point_radius * border_thickness_relative;

    if (dist_from_center < point_radius - border_abs_thickness) {
        // Inside the main part of the point
        outColor = vec4(1.0, 1.0, 1.0, 1.0); // White
    } else if (dist_from_center < point_radius) {
        // In the border region of the point
        outColor = vec4(0.0, 0.0, 0.0, 1.0); // Black
    } else {
        // Outside the circular point (corners of the square point primitive)
        discard;
    }
}
