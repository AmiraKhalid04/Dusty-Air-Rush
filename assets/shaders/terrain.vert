#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;      // .r = height [0,1] packed by terrain-utils
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 world_pos;
} vs_out;

uniform mat4 transform;      // MVP
uniform mat4 model;          // M only, for world-space normal/pos

void main() {
    vec4 world = model * vec4(position, 1.0);
    gl_Position    = transform * vec4(position, 1.0);
    vs_out.color     = color;
    vs_out.tex_coord = tex_coord;
    // Transform normal to world space (use inverse-transpose for non-uniform scale)
    vs_out.normal    = normalize(mat3(transpose(inverse(model))) * normal);
    vs_out.world_pos = world.xyz;
}
