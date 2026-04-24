#version 330 core

#define DIRECTIONAL 0
#define POINT       1
#define SPOT        2

struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    vec3 attenuation;
    vec2 cone_angles;
};

#define MAX_LIGHTS 8

uniform Light lights[MAX_LIGHTS];
uniform int light_count;

struct Sky {
    vec3 top, horizon, bottom;
};

uniform Sky sky;

vec3 compute_sky_light(vec3 normal){
    vec3 extreme = normal.y > 0 ? sky.top : sky.bottom;
    return mix(sky.horizon, extreme, normal.y * normal.y);
}

struct Material {
    sampler2D albedo;
    sampler2D specular;
    sampler2D roughness;
    sampler2D ambient_occlusion;
    sampler2D emissive;
};

uniform Material material;
uniform vec4 tint;
uniform float alphaThreshold;
uniform sampler2D shadow_map;

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view;
    vec3 world;
    vec4 fragPosLightSpace;
} fs_in;

out vec4 frag_color;

float lambert(vec3 normal, vec3 world_to_light_direction) {
    return max(0.0, dot(normal, world_to_light_direction));
}

float phong(vec3 reflected, vec3 view, float shininess) {
    return pow(max(0.0, dot(reflected, view)), shininess);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
        
    float currentDepth = projCoords.z;
    float bias = max(0.025 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // Percentage-Closer Filtering (PCF)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadow_map, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    vec4 m_albedo = texture(material.albedo, fs_in.tex_coord) * tint * fs_in.color;
    if(m_albedo.a < alphaThreshold)
        discard;
        
    vec3 normal = normalize(fs_in.normal);
    vec3 view = normalize(fs_in.view);
    
    vec3 ambient_light = compute_sky_light(normal);

    vec3 diffuse = m_albedo.rgb;
    vec3 specular = texture(material.specular, fs_in.tex_coord).rgb;
    float roughness = texture(material.roughness, fs_in.tex_coord).r;
    vec3 ambient = diffuse * texture(material.ambient_occlusion, fs_in.tex_coord).r;
    vec3 emissive = texture(material.emissive, fs_in.tex_coord).rgb;

    float shininess = 2.0 / pow(clamp(roughness, 0.001, 0.999), 4.0) - 2.0;
    
    vec3 world_to_light_dir;
    vec3 color = emissive + ambient_light * ambient;

    for(int light_idx = 0; light_idx < min(MAX_LIGHTS, light_count); light_idx++){
        Light light = lights[light_idx];
        float attenuation = 1.0;
        float shadow = 0.0;
        
        if(light.type == DIRECTIONAL){
            world_to_light_dir = -light.direction;
            shadow = ShadowCalculation(fs_in.fragPosLightSpace, normal, world_to_light_dir);
        } else if(light.type == POINT){
            world_to_light_dir = light.position - fs_in.world;
            float d = length(world_to_light_dir);
            world_to_light_dir /= d;
            attenuation = 1.0 / dot(light.attenuation, vec3(1.0, d, d*d));
        } else if(light.type == SPOT){
            world_to_light_dir = light.position - fs_in.world;
            float d = length(world_to_light_dir);
            world_to_light_dir /= d;
            attenuation = 1.0 / max(dot(light.attenuation, vec3(1.0, d, d*d)), 0.00001);
            float angle = acos(clamp(dot(-world_to_light_dir, normalize(light.direction)), -1.0, 1.0));
            attenuation *= 1.0 - smoothstep(light.cone_angles.x, light.cone_angles.y, angle);
        }

        vec3 computed_diffuse = light.color * diffuse * lambert(normal, world_to_light_dir);

        vec3 reflected = reflect(-world_to_light_dir, normal);
        vec3 computed_specular = light.color * specular * phong(reflected, view, shininess);

        color += (computed_diffuse + computed_specular) * attenuation * (1.0 - shadow);
    }
    
    frag_color = vec4(color, m_albedo.a);
}