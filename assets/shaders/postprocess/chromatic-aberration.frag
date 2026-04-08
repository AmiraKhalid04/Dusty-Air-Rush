#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;
out vec4 frag_color;

// How far (in the texture space) is the distance (on the x-axis) between
// the pixels from which the red/green (or green/blue) channels are sampled
#define STRENGTH 0.005

// Chromatic aberration mimics some old cameras where the lens disperses light
// differently based on its wavelength. In this shader, we will implement a
// cheap version of that effect 

void main(){
    float r = texture(tex, vec2(tex_coord.x - STRENGTH, tex_coord.y)).r;
    float g = texture(tex, tex_coord).g;
    float b = texture(tex, vec2(tex_coord.x + STRENGTH, tex_coord.y)).b;
    float a = texture(tex, tex_coord).a;

    frag_color = vec4(r, g, b, a);
}