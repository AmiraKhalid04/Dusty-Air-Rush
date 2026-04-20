#version 330 core

in Varyings {
    vec4  color;
    vec2  tex_coord;
    vec3  normal;
    vec3  world_pos;
} fs_in;

out vec4 frag_color;

// ---------------------------------------------------------------
// Biome textures
// ---------------------------------------------------------------
uniform sampler2D tex_water;
uniform sampler2D tex_sand;
uniform sampler2D tex_grass;
uniform sampler2D tex_rock;
uniform sampler2D tex_snow;

// ---------------------------------------------------------------
// Must match heightScale in mesh config (20.0)
// h = world_pos.y / u_heightScale → always [0,1]
// ---------------------------------------------------------------
uniform float u_heightScale;

// ---------------------------------------------------------------
// Lighting & tint
// ---------------------------------------------------------------
uniform vec4  tint;
uniform vec3  sun_direction;
uniform vec3  sun_color;
uniform vec3  ambient_color;
uniform float time;

// ---------------------------------------------------------------
// Biome thresholds — tuned for a full [0,1] normalized range
// Water:  0%  – 20%
// Sand:  20%  – 30%
// Grass: 30%  – 60%
// Rock:  60%  – 80%
// Snow:  80%  – 100%
// ---------------------------------------------------------------
const float H_WATER_SHORE = 0.30;
const float H_SAND_END    = 0.35;
const float H_GRASS_END   = 0.45;
const float H_ROCK_END    = 0.80;

void main() {
    // Recover normalized height from world Y — full float precision
    // Recover raw h
    float h = clamp((fs_in.world_pos.y + u_heightScale * 0.5) / u_heightScale, 0.0, 1.0);

    // Apply a power curve to redistribute — values < 0.5 push down, > 0.5 push up
    // This spreads a bimodal distribution into all 5 biome bands
    h = smoothstep(0.0, 1.0, h);  // soften the extremes
    
    vec2 uv       = fs_in.tex_coord;
    vec2 water_uv = uv + vec2(time * 0.02, time * 0.015);

    // Sample biomes
    vec4 c_water = texture(tex_water, water_uv);
    vec4 c_sand  = texture(tex_sand,  uv);
    vec4 c_grass = texture(tex_grass, uv);
    vec4 c_rock  = texture(tex_rock,  uv);
    vec4 c_snow  = texture(tex_snow,  uv);

    // Slope-based rock (cliffs always show rock)
    vec3  N         = normalize(fs_in.normal);
    float slope     = 1.0 - abs(dot(N, vec3(0.0, 1.0, 0.0)));
    float slopeRock = smoothstep(0.45, 0.70, slope);

    // Height weights — each biome occupies a clear band
    float w_water = 1.0 - smoothstep(0.0,              H_WATER_SHORE,      h);
    float w_sand  = smoothstep(0.15,   H_WATER_SHORE,  h)
                  * (1.0 - smoothstep(H_SAND_END-0.05, H_SAND_END,         h));
    float w_grass = smoothstep(H_SAND_END-0.05, H_SAND_END,   h)
                  * (1.0 - smoothstep(H_GRASS_END-0.08, H_GRASS_END,       h));
    float w_rock  = smoothstep(H_GRASS_END-0.08, H_GRASS_END, h)
                  * (1.0 - smoothstep(H_ROCK_END-0.08,  H_ROCK_END,        h));
    float w_snow  = smoothstep(H_ROCK_END-0.08,  H_ROCK_END,  h);

    // Steep slopes override to rock
    w_grass = w_grass * (1.0 - slopeRock);
    w_snow  = w_snow  * (1.0 - slopeRock);
    w_rock  = clamp(w_rock + slopeRock * (w_grass + w_snow), 0.0, 1.0);

    vec4 terrain_color = w_water * c_water
                       + w_sand  * c_sand
                       + w_grass * c_grass
                       + w_rock  * c_rock
                       + w_snow  * c_snow;

    // Diffuse lighting
    vec3  L       = normalize(sun_direction);
    float NL      = max(dot(N, L), 0.0);
    vec3  lighting = ambient_color + sun_color * NL;

    // Water specularity
    vec3  V     = normalize(-fs_in.world_pos);
    vec3  Hvec  = normalize(L + V);
    float spec  = pow(max(dot(N, Hvec), 0.0), 64.0) * w_water * 0.6;

    frag_color = tint * terrain_color * vec4(lighting, 1.0)
               + vec4(vec3(spec), 0.0);
}