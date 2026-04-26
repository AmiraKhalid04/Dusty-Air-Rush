#version 330

uniform sampler2D tex;
uniform sampler2D depth_tex;

uniform float near_plane;
uniform float far_plane;
uniform vec3 fog_color;
uniform float fog_density;
// fog_gradient is ignored in this ultra-fast linear version

in vec2 tex_coord;
out vec4 frag_color;

void main(){
    float depth = texture(depth_tex, tex_coord).r;
    
    // Highly optimized linear depth math
    float c = 1.0 - (near_plane / far_plane);
    float linearDepth = near_plane / (1.0 - depth * c);
    
    // ULTRA-FAST Linear Fog (no pow, no exp)
    // Simply scales linearly with distance
    float fogFactor = clamp(linearDepth * fog_density, 0.0, 1.0);
    
    if (fogFactor > 0.99) {
        frag_color = vec4(fog_color, 1.0);
        return;
    }
    
    vec4 sceneColor = texture(tex, tex_coord);
    frag_color = vec4(mix(sceneColor.rgb, fog_color, fogFactor), sceneColor.a);
}
