#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;

out vec4 frag_color;

// Vignette is a postprocessing effect that darkens the corners of the screen
// to grab the attention of the viewer towards the center of the screen

void main(){
    vec4 color = texture(tex, tex_coord);
    float vignette = 1.0 / (1.0 + pow(2.0 * tex_coord.x - 1.0, 2.0) + pow(2.0 * tex_coord.y - 1.0, 2.0));
    frag_color = vec4(color.rgb * vignette, color.a);
}