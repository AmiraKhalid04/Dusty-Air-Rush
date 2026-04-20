#version 330 core

uniform sampler2D tex;
uniform bool has_texture;
uniform float alphaThreshold;

in vec2 tex_coord;

void main()
{
    if (has_texture) {
        if (texture(tex, tex_coord).a < alphaThreshold) {
            discard;
        }
    }
}