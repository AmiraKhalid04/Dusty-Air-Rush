#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;

out Varyings {
    vec4  color;
    vec2  tex_coord;
} vs_out;

uniform mat4 transform;   // MVP
uniform mat4 model;
uniform vec3 camera_pos;  // for billboarding

void main() {
    // Billboarding: rotate the flat cloud quad to always face the camera
    // The cloud entity's Y stays fixed; only XZ faces camera.
    vec3 world_center = vec3(model[3]);   // translation of this cloud instance

    vec3 to_cam = normalize(camera_pos - world_center);
    to_cam.y = 0.0;                        // keep cloud flat (horizontal billboard)
    to_cam = normalize(to_cam);

    vec3 up    = vec3(0, 1, 0);
    vec3 right = normalize(cross(up, to_cam));

    // Reconstruct world position using billboard axes
    float scale = length(vec3(model[0]));  // extract uniform scale from model
    vec3 world_pos = world_center
        + right * position.x * scale
        + up    * position.z * scale;      // plane is in XZ, map Z→Y for height

    gl_Position    = transform * vec4(world_pos, 1.0) ;

    // Slight UV drift for texture animation (no time here; done in frag)
    vs_out.color     = color;
    vs_out.tex_coord = tex_coord;
}
