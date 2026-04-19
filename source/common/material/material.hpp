#pragma once

#include "pipeline-state.hpp"
#include "../texture/texture2d.hpp"
#include "../texture/sampler.hpp"
#include "../shader/shader.hpp"

#include <glm/vec4.hpp>
#include <json/json.hpp>
#include <iostream>
#include "../asset-loader.hpp"

namespace our
{

    // This is the base class for all the materials
    // It contains the 3 essential components required by any material
    // 1- The pipeline state when drawing objects using this material
    // 2- The shader program used to draw objects using this material
    // 3- Whether this material is transparent or not
    // Materials that send uniforms to the shader should inherit from the is material and add the required uniforms
    class Material
    {
    public:
        PipelineState pipelineState;
        ShaderProgram *shader = nullptr;
        bool transparent = false;

        // This function does 2 things: setup the pipeline state and set the shader program to be used
        virtual void setup() const;
        // This function read a material from a json object
        virtual void deserialize(const nlohmann::json &data);
    };

    // This material adds a uniform for a tint (a color that will be sent to the shader)
    // An example where this material can be used is when the whole object has only color which defined by tint
    class TintedMaterial : public Material
    {
    public:
        glm::vec4 tint = {1.0f, 1.0f, 1.0f, 1.0f};

        void setup() const override;
        void deserialize(const nlohmann::json &data) override;
    };

    // This material adds two uniforms (besides the tint from Tinted Material)
    // The uniforms are:
    // - "tex" which is a Sampler2D. "texture" and "sampler" will be bound to it.
    // - "alphaThreshold" which defined the alpha limit below which the pixel should be discarded
    // An example where this material can be used is when the object has a texture
    class TexturedMaterial : public TintedMaterial
    {
    public:
        Texture2D *texture = nullptr;
        Sampler *sampler = nullptr;
        float alphaThreshold = 0.0f;

        void setup() const override;
        void deserialize(const nlohmann::json &data) override;
    };

    class LitMaterial : public TexturedMaterial
    {
    public:
        Texture2D *albedo = nullptr;
        Texture2D *specular = nullptr;
        Texture2D *roughness = nullptr;
        Texture2D *ambient_occlusion = nullptr;
        Texture2D *emissive = nullptr;

        void setup() const override;
        void deserialize(const nlohmann::json &data) override;
    };

    class TerrainMaterial : public TintedMaterial
    {
    public:
        Texture2D *tex_water = nullptr;
        Texture2D *tex_sand = nullptr;
        Texture2D *tex_grass = nullptr;
        Texture2D *tex_rock = nullptr;
        Texture2D *tex_snow = nullptr;
        Sampler *sampler = nullptr;

        glm::vec4 tint = glm::vec4(1.0f);
        glm::vec3 sun_direction = glm::normalize(glm::vec3(0.6f, 1.0f, 0.4f));
        glm::vec3 sun_color = glm::vec3(1.0f, 0.95f, 0.8f);
        glm::vec3 ambient_color = glm::vec3(0.25f, 0.30f, 0.40f);
        float time = 0.0f; // update each frame: terrainMat->time += deltaTime
        float heightScale = 20.0f; // must match mesh config heightScale

        void setup() const override
        {
            pipelineState.setup();
            shader->use();

            // The bind lambda must also call shader->set() AFTER binding
            auto bind = [&](Texture2D *t, int unit, const char *name)
            {
                if (!t)
                {
                    std::cerr << "[TerrainMaterial] NULL texture for: " << name << "\n";
                    return;
                }
                glActiveTexture(GL_TEXTURE0 + unit);
                t->bind();
                if (sampler)
                    sampler->bind(unit);
                shader->set(name, unit); // <-- this line is critical
            };

            bind(tex_water, 0, "tex_water");
            bind(tex_sand, 1, "tex_sand");
            bind(tex_grass, 2, "tex_grass");
            bind(tex_rock, 3, "tex_rock");
            bind(tex_snow, 4, "tex_snow");

            shader->set("tint", tint);
            shader->set("sun_direction", sun_direction);
            shader->set("sun_color", sun_color);
            shader->set("ambient_color", ambient_color);
            shader->set("time", time);
            shader->set("u_heightScale", heightScale);
        }

        void deserialize(const nlohmann::json &data) override;
       
        // The renderer also needs to set "model" uniform separately:
        //   terrainMat->shader->set("model", command.localToWorld);
    };

    class CloudMaterial : public Material
    {
    public:
        Texture2D *texture = nullptr;
        Sampler *sampler = nullptr;
        glm::vec4 tint = glm::vec4(1.0f);
        float time = 0.0f;

        void setup() const override
        {
            pipelineState.setup();
            shader->use();
            if (texture)
            {
                glActiveTexture(GL_TEXTURE0);
                texture->bind();
                if (sampler)
                    sampler->bind(0);
                shader->set("tex", 0);
            }
            shader->set("tint", tint);
            shader->set("time", time);
        }
    };

    // This function returns a new material instance based on the given type
    inline Material *createMaterialFromType(const std::string &type)
    {
        if (type == "tinted")
        {
            return new TintedMaterial();
        }
        else if (type == "textured")
        {
            return new TexturedMaterial();
        }
        else if (type == "lit")
        {
            return new LitMaterial();
        }
        else if (type == "terrain")
        {
            return new TerrainMaterial();
        }
        else if (type == "cloud")
        {
            return new CloudMaterial();
        }
        else
        {
            return new Material();
        }
    }
}