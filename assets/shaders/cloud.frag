#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform sampler2D tex;        // cloud noise texture (white noise / perlin)
uniform vec4      tint;       // overall tint (usually white)
uniform float     time;       // for slow UV drift

// Edge softness: clouds fade out at the quad edges
float edgeFade(vec2 uv) {
    vec2 d = abs(uv - 0.5) * 2.0;          // 0 at center, 1 at edge
    return 1.0 - smoothstep(0.6, 1.0, max(d.x, d.y));
}

void main() {
    // Slowly drift the cloud texture
    vec2 uv1 = fs_in.tex_coord + vec2(time * 0.008,  time * 0.003);
    vec2 uv2 = fs_in.tex_coord + vec2(time * -0.005, time * 0.006) + 0.5;

    // Two octaves for a fluffy look
    float n1 = texture(tex, uv1).r;
    float n2 = texture(tex, uv2 * 1.7).r * 0.5;
    float noise = clamp(n1 + n2 - 0.3, 0.0, 1.0);   // threshold + combine

    // Fade at quad edges so clouds don't have hard borders
    float alpha = noise * edgeFade(fs_in.tex_coord);

    // Discard very thin wisps to avoid z-fighting artifacts
    if (alpha < 0.05) discard;

    vec3 cloud_color = mix(vec3(0.85, 0.90, 1.0),   // shadow (blue-grey)
                           vec3(1.0,  1.0,  1.0),   // bright white
                           noise);

    frag_color = tint * vec4(cloud_color, alpha);
}
