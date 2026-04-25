#version 330

// The texture holding the scene pixels
uniform sampler2D tex;
// The texture holding the depth buffer
uniform sampler2D depth_tex;

// Camera parameters for depth reconstruction
uniform float near_plane;
uniform float far_plane;

// Fog parameters
uniform vec3 fog_color;
uniform float fog_density;
uniform float fog_gradient;

in vec2 tex_coord;
out vec4 frag_color;

// Reconstruct linear depth from the depth texture
float getLinearDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Convert from [0, 1] to [-1, 1]
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

// Calculate fog factor based on distance
float calculateFogFactor(float distance) {
    // Standard exponential fog formula
    // We multiply distance by density first, then apply the gradient (pow)
    // This allows small density values like 0.01 to work correctly with large distances
    float fog = exp(-pow(distance * fog_density, fog_gradient));
    return 1.0 - clamp(fog, 0.0, 1.0);
}

void main(){
    // Sample the scene color
    vec4 sceneColor = texture(tex, tex_coord);
    
    // Sample the depth
    float depth = texture(depth_tex, tex_coord).r;

    
    // Get linear depth
    float linearDepth = getLinearDepth(depth);
    
    // Calculate fog factor
    float fogFactor = calculateFogFactor(linearDepth);
    
    // Clamp fog factor between 0 and 1
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    // Blend scene color with fog color
    // More fog (higher fogFactor) = more fog color
    vec3 finalColor = mix(sceneColor.rgb, fog_color, fogFactor);
    
    frag_color = vec4(finalColor, sceneColor.a);
}
