#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../components/light.hpp"
#include "../deserialize-utils.hpp"
#include <GLFW/glfw3.h>

#include "../material/material.hpp"
#include "../components/dusty.hpp"

namespace our
{

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json &config)
    {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Initialize Shadow Map FBO and Texture
        glGenFramebuffers(1, &shadowMapFBO);
        shadowMapTexture = new Texture2D();
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        shadowMapTexture->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMapTexture->getOpenGLName(), 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        shadowShader = AssetLoader<ShaderProgram>::get("shadow");

        if (config.contains("sky_lighting"))
        {
            auto skyLight = config["sky_lighting"];
            skyTop = skyLight.value("top", glm::vec3(1.0f));
            skyHorizon = skyLight.value("horizon", glm::vec3(1.0f));
            skyBottom = skyLight.value("bottom", glm::vec3(1.0f));
        }

        if (auto runwayMat = AssetLoader<Material>::get("runway_light"))
        {
            if (auto tinted = dynamic_cast<TintedMaterial *>(runwayMat))
            {
                runwayLightBaseTint = tinted->tint;
            }
        }

        if (config.contains("sky"))
        {
            // The sky shader is a ShaderToy-style fullscreen shader.
            // We render it at HALF resolution into a dedicated FBO, then upscale
            // with a blit pass. This gives ~4x GPU savings on the expensive raymarching.
            glm::ivec2 skyRes = windowSize / 2;

            // --- Half-resolution sky FBO ---
            glGenFramebuffers(1, &skyFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, skyFrameBuffer);

            glGenTextures(1, &skyColorTexture);
            glBindTexture(GL_TEXTURE_2D, skyColorTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, skyRes.x, skyRes.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyColorTexture, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // --- Sky raymarching shader (runs at half-res) ---
            ShaderProgram *skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/sky.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            PipelineState skyPipelineState{};
            skyPipelineState.depthTesting.enabled = false;
            skyPipelineState.faceCulling.enabled = false;
            skyPipelineState.blending.enabled = false;
            skyPipelineState.colorMask = {true, true, true, true};
            skyPipelineState.depthMask = false;

            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;

            // --- Blit shader (upscales half-res sky to full screen) ---
            skyBlitShader = new ShaderProgram();
            skyBlitShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            skyBlitShader->attach("assets/shaders/blit.frag", GL_FRAGMENT_SHADER);
            skyBlitShader->link();

            // VAO for the fullscreen triangle draw calls (both sky and blit)
            glGenVertexArrays(1, &skyVertexArray);
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess"))
        {
            // TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            // TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            //  Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            //  The depth format can be (Depth component with 24 bits).
            colorTarget = new Texture2D();
            colorTarget->bind();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowSize.x, windowSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);

            depthTarget = new Texture2D();
            depthTarget->bind();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, windowSize.x, windowSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);

            // TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler *postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram *postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;

            // Load fog parameters if they exist in config
            if (config.contains("fog"))
            {
                auto fog = config["fog"];
                fogColor = fog.value("color", glm::vec3(0.5f));
                fogDensity = fog.value("density", 0.01f);
            }
        }
    }

    void ForwardRenderer::destroy()
    {
        if (shadowMapFBO)
            glDeleteFramebuffers(1, &shadowMapFBO);
        if (shadowMapTexture)
            delete shadowMapTexture;

        // Delete all objects related to the sky
        if (skyMaterial)
        {
            glDeleteVertexArrays(1, &skyVertexArray);
            glDeleteFramebuffers(1, &skyFrameBuffer);
            glDeleteTextures(1, &skyColorTexture);
            delete skyBlitShader;
            delete skyMaterial->shader;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial)
        {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World *world)
    {
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent *camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();

        struct LightData
        {
            LightComponent *light;
            glm::mat4 localToWorld;
        };
        std::vector<LightData> activeLights;

        float time = std::fmod((float)glfwGetTime() * 0.02f, 1.0f);
        float xk = std::pow(time, 1.5f);
        float toD = xk / (xk + std::pow(1.0f - time, 1.5f));
        toD = -toD * 6.283853f - 1.5708f + 0.1f;
        glm::vec3 sunDir = glm::normalize(glm::vec3(std::sin(toD) * 0.4f + 0.4f, std::sin(toD) + 0.69f, std::cos(toD)));
        float sunUp = glm::max(glm::dot(sunDir, glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f);
        float sunStrength = glm::clamp(sunUp * sunUp * sunUp * 6.0f, 0.0f, 1.0f);
        float directionalStrength = glm::max(sunStrength, 0.05f); // Keep a bit of light at night
        float lampStrength = 1.0f - glm::smoothstep(0.0f, 0.2f, sunStrength);
        
        // --- Runway Light Visual Glow ---
        // We modulate the material tint of the lamps so they look like they "turn on"
        // and pulsate slightly at night.
        if (auto runwayMat = AssetLoader<Material>::get("runway_light")) {
            if (auto tinted = dynamic_cast<TintedMaterial*>(runwayMat)) {
                float pulse = 1.0f;
                if (lampStrength > 0.0f) {
                    pulse = 1.0f + 0.15f * std::sin((float)glfwGetTime() * 4.0f);
                }
                float brightness = 1.0f + 1.2f * lampStrength * pulse;
                tinted->tint = glm::vec4(glm::vec3(runwayLightBaseTint) * brightness, runwayLightBaseTint.a);
            }
        }

        for (auto entity : world->getEntities())
        {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera)
                camera = entity->getComponent<CameraComponent>();

            // If this entity has a light component
            if (auto light = entity->getComponent<LightComponent>(); light)
            {
                if (light->lightType != LightType::SPOT)
                {
                    activeLights.push_back({light, entity->getLocalToWorldMatrix()});
                }
                else 
                {
                    // For spotlights, check if they are attached to Dusty and if headlights are on
                    our::Entity* current = entity;
                    bool isHeadlightsOn = true; // default to true
                    // loop up the parent hierarchy to find a DustyComponent, if it exists
                    while (current) {
                        if (auto dusty = current->getComponent<DustyComponent>()) {
                            isHeadlightsOn = dusty->isHeadlightsOn;
                            break;
                        }
                        current = current->parent;
                    }
                    if (isHeadlightsOn) {
                        activeLights.push_back({light, entity->getLocalToWorldMatrix()});
                    }
                }
            }

            // If this entity has a mesh renderer component
            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer)
            {
                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                // if it is transparent, we add it to the transparent commands list
                if (command.material->transparent)
                {
                    transparentCommands.push_back(command);
                }
                else
                {
                    // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }

            // Debug rendering for Colliders
            if (auto collider = entity->getComponent<ColliderComponent>(); collider)
            {
                Material *debugMat = our::AssetLoader<Material>::get("debug_wireframe");
                // if (debugMat)
                // {
                //     RenderCommand command;
                //     glm::mat4 baseTransform = glm::translate(entity->getLocalToWorldMatrix(), collider->center);

                //     if (collider->shapeType == ColliderType::Sphere)
                //     {
                //         command.localToWorld = glm::scale(baseTransform, glm::vec3(collider->sphereRadius));
                //         command.mesh = our::AssetLoader<Mesh>::get("sphere");
                //     }
                //     else
                //     {
                //         command.localToWorld = glm::scale(baseTransform, collider->aabbExtents);
                //         command.mesh = our::AssetLoader<Mesh>::get("cube");
                //     }

                //     command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                //     command.material = debugMat;

                //     // Wireframes look better in transparent pass to overlay gracefully
                //     transparentCommands.push_back(command);
                // }
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if (camera == nullptr)
            return;

        // TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        //  HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one
        auto M = camera->getOwner()->getLocalToWorldMatrix();
        glm::vec3 cameraForward = glm::vec3(M * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand &first, const RenderCommand &second)
                  {
            //TODO: (Req 9) Finish this function
            // HINT: the following return should return true "first" should be drawn before "second". 
            return glm::dot(first.center, cameraForward) > glm::dot(second.center, cameraForward); });

        // =========================================================
        // SHADOW PASS
        // =========================================================
        glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
        glm::vec3 cameraPosition = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

        // Sort active lights to prioritize Directional, then Spot, then nearest Point lights
        std::sort(activeLights.begin(), activeLights.end(), [&cameraPosition](const LightData &a, const LightData &b)
                  {
            if (a.light->lightType == LightType::DIRECTIONAL && b.light->lightType != LightType::DIRECTIONAL) return true;
            if (b.light->lightType == LightType::DIRECTIONAL && a.light->lightType != LightType::DIRECTIONAL) return false;
            
            if (a.light->lightType == LightType::SPOT && b.light->lightType != LightType::SPOT) return true;
            if (b.light->lightType == LightType::SPOT && a.light->lightType != LightType::SPOT) return false;
            
            glm::vec3 posA = glm::vec3(a.localToWorld * glm::vec4(0, 0, 0, 1));
            glm::vec3 posB = glm::vec3(b.localToWorld * glm::vec4(0, 0, 0, 1));
            return glm::distance(posA, cameraPosition) < glm::distance(posB, cameraPosition); });

        if (shadowShader && shadowMapFBO != 0)
        {
            glm::vec3 lightDir(0.0f, -1.0f, 0.0f);
            bool hasDirLight = false;
            for (auto &ld : activeLights)
            {
                if (ld.light->lightType == LightType::DIRECTIONAL)
                {
                    lightDir = -sunDir;
                    hasDirLight = true;
                    break;
                }
            }

            if (hasDirLight)
            {
                // Render to the shadow map FBO
                glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
                glViewport(0, 0, 2048, 2048);

                // FIX: Ensure depth mask is true so we can actually clear and write to the depth buffer!
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
                glDisable(GL_CULL_FACE); // Prevent backface culling issues from thinning rings

                glClear(GL_DEPTH_BUFFER_BIT);

                // Configure light space matrix for an Orthographic directional light calculation
                float bounds = 80.0f; // Scale to cover the moving player
                glm::mat4 lightProjection = glm::ortho(-bounds, bounds, -bounds, bounds, 1.0f, 200.0f);
                glm::vec3 lightPos = cameraPosition - lightDir * 100.0f; // Push light back relative to camera

                // Avoid singularity if lightDir is straight down or straight up!
                glm::vec3 up(0.0f, 1.0f, 0.0f);
                if (glm::abs(lightDir.y) > 0.999f)
                    up = glm::vec3(0.0f, 0.0f, 1.0f);

                glm::mat4 lightView = glm::lookAt(lightPos, cameraPosition, up);
                lightSpaceMatrix = lightProjection * lightView;

                shadowShader->use();
                shadowShader->set("lightSpaceMatrix", lightSpaceMatrix);

                // Draw Opaque items to populate depth
                glEnable(GL_DEPTH_TEST);
                for (const auto &cmd : opaqueCommands)
                {
                    if (cmd.material != skyMaterial)
                    {
                        shadowShader->set("M", cmd.localToWorld);

                        Texture2D *alphaTex = nullptr;
                        float alphaThresh = 0.5f; // Default threshold to chop out exactly 50%

                        if (auto litMat = dynamic_cast<LitMaterial *>(cmd.material))
                        {
                            alphaTex = litMat->albedo;
                            alphaThresh = litMat->alphaThreshold > 0.0f ? litMat->alphaThreshold : 0.5f;
                        }
                        else if (auto texMat = dynamic_cast<TexturedMaterial *>(cmd.material))
                        {
                            alphaTex = texMat->texture;
                            alphaThresh = texMat->alphaThreshold > 0.0f ? texMat->alphaThreshold : 0.5f;
                        }

                        if (alphaTex)
                        {
                            shadowShader->set("has_texture", true);
                            shadowShader->set("alphaThreshold", alphaThresh);
                            glActiveTexture(GL_TEXTURE5);
                            alphaTex->bind();
                            shadowShader->set("tex", 5);
                        }
                        else
                        {
                            shadowShader->set("has_texture", false);
                        }

                        cmd.mesh->draw();
                    }
                }

                // Unbind our shadow map and restore defaults
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }

        // =========================================================
        // BEAUTY PASS
        // =========================================================
        // TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        // TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        glViewport(0, 0, windowSize.x, windowSize.y);

        // TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClearDepth(1.0f);

        // TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // If there is a postprocess material, bind the framebuffer
        if (postprocessMaterial)
        {
            // TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        // TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cameraPosition = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

        // --- Sky: two-pass half-resolution rendering ---
        // Pass 1: run the expensive raymarching shader into the half-res FBO
        // Pass 2: upscale the result back to the main framebuffer with a blit
        if (this->skyMaterial)
        {
            auto skyM = camera->getOwner()->getLocalToWorldMatrix();
            glm::vec3 skyFwd = glm::normalize(glm::vec3(skyM * glm::vec4(0, 0, -1, 0)));
            glm::vec3 skyUp = glm::normalize(glm::vec3(skyM * glm::vec4(0, 1, 0, 0)));
            glm::vec3 skyEye = glm::vec3(0.0f, 200.0f, 0.0f);
            glm::ivec2 skyRes = windowSize / 2;

            // --- Pass 1: render sky at half resolution ---
            glBindFramebuffer(GL_FRAMEBUFFER, skyFrameBuffer);
            glViewport(0, 0, skyRes.x, skyRes.y);

            this->skyMaterial->setup();
            this->skyMaterial->shader->set("iTime", (float)glfwGetTime());
            this->skyMaterial->shader->set("iResolution", glm::vec2(skyRes)); // half-res so UVs stay correct
            this->skyMaterial->shader->set("eye", skyEye);
            this->skyMaterial->shader->set("forward", skyFwd);
            this->skyMaterial->shader->set("up", skyUp);

            glBindVertexArray(skyVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            // --- Pass 2: blit the half-res result to the main framebuffer at full size ---
            // Restore main (or postprocess) framebuffer and full viewport
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessMaterial ? postprocessFrameBuffer : 0);
            glViewport(0, 0, windowSize.x, windowSize.y);

            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            skyBlitShader->use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, skyColorTexture);
            skyBlitShader->set("tex", 0);

            glBindVertexArray(skyVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            // Re-enable depth writes for opaque geometry
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
        }

        // TODO: (Req 9) Draw all the opaque commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (const RenderCommand &command : opaqueCommands)
        {
            command.material->setup();
            glm::mat4 transform = VP * command.localToWorld;
            command.material->shader->set("transform", transform);

            // Lighting & Advanced Transforms
            if (auto litMat = dynamic_cast<LitMaterial *>(command.material); litMat)
            {
                command.material->shader->set("VP", VP);
                command.material->shader->set("M", command.localToWorld);
                command.material->shader->set("M_IT", glm::transpose(glm::inverse(command.localToWorld)));
                command.material->shader->set("camera_position", cameraPosition);

                command.material->shader->set("sky.top", skyTop);
                command.material->shader->set("sky.horizon", skyHorizon);
                command.material->shader->set("sky.bottom", skyBottom);

                int maxLights = std::min((int)activeLights.size(), 16);
                command.material->shader->set("light_count", maxLights);
                for (int i = 0; i < maxLights; i++)
                {
                    std::string prefix = "lights[" + std::to_string(i) + "].";
                    command.material->shader->set(prefix + "type", (int)activeLights[i].light->lightType);

                    glm::vec3 lightColor = activeLights[i].light->diffuse;
                    glm::vec3 dir = activeLights[i].localToWorld * glm::vec4(0, 0, -1, 0);
                    if (activeLights[i].light->lightType == LightType::DIRECTIONAL)
                    {
                        lightColor *= directionalStrength;
                        dir = -sunDir;
                    } else if (activeLights[i].light->lightType == LightType::POINT) {
                        lightColor *= lampStrength;
                    }
                    command.material->shader->set(prefix + "color", lightColor);
                    command.material->shader->set(prefix + "attenuation", activeLights[i].light->attenuation);
                    glm::vec3 pos = activeLights[i].localToWorld * glm::vec4(0, 0, 0, 1);
                    command.material->shader->set(prefix + "position", pos);
                    command.material->shader->set(prefix + "direction", glm::normalize(dir));
                    command.material->shader->set(prefix + "cone_angles", activeLights[i].light->cone_angles);
                }

                // Pass shadow map parameters
                command.material->shader->set("lightSpaceMatrix", lightSpaceMatrix);
                if (shadowMapTexture)
                {
                    glActiveTexture(GL_TEXTURE5);
                    shadowMapTexture->bind();
                    command.material->shader->set("shadow_map", 5);
                }
            }

            command.mesh->draw();
        }

        // TODO: (Req 9) Draw all the transparent commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (const RenderCommand &command : transparentCommands)
        {
            command.material->setup();
            glm::mat4 transform = VP * command.localToWorld;
            command.material->shader->set("transform", transform);

            // Lighting & Advanced Transforms
            if (auto litMat = dynamic_cast<LitMaterial *>(command.material); litMat)
            {
                command.material->shader->set("VP", VP);
                command.material->shader->set("M", command.localToWorld);
                command.material->shader->set("M_IT", glm::transpose(glm::inverse(command.localToWorld)));
                command.material->shader->set("camera_position", cameraPosition);

                command.material->shader->set("sky.top", skyTop);
                command.material->shader->set("sky.horizon", skyHorizon);
                command.material->shader->set("sky.bottom", skyBottom);

                int maxLights = std::min((int)activeLights.size(), 16);
                command.material->shader->set("light_count", maxLights);
                for (int i = 0; i < maxLights; i++)
                {
                    std::string prefix = "lights[" + std::to_string(i) + "].";
                    command.material->shader->set(prefix + "type", (int)activeLights[i].light->lightType);

                    glm::vec3 lightColor = activeLights[i].light->diffuse;
                    glm::vec3 dir = activeLights[i].localToWorld * glm::vec4(0, 0, -1, 0);
                    if (activeLights[i].light->lightType == LightType::DIRECTIONAL)
                    {
                        lightColor *= directionalStrength;
                        dir = -sunDir;
                    } else if (activeLights[i].light->lightType == LightType::POINT) {
                        lightColor *= lampStrength;
                    }
                    command.material->shader->set(prefix + "color", lightColor);
                    command.material->shader->set(prefix + "attenuation", activeLights[i].light->attenuation);
                    glm::vec3 pos = activeLights[i].localToWorld * glm::vec4(0, 0, 0, 1);
                    command.material->shader->set(prefix + "position", pos);
                    command.material->shader->set(prefix + "direction", glm::normalize(dir));
                    command.material->shader->set(prefix + "cone_angles", activeLights[i].light->cone_angles);
                }

                // Pass shadow map parameters
                command.material->shader->set("lightSpaceMatrix", lightSpaceMatrix);
                if (shadowMapTexture)
                {
                    glActiveTexture(GL_TEXTURE5);
                    shadowMapTexture->bind();
                    command.material->shader->set("shadow_map", 5);
                }
            }

            command.mesh->draw();
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial)
        {
            // TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();

            // Bind the depth texture to texture unit 1 and pass it to the shader
            if (depthTarget && postprocessMaterial->shader)
            {
                glActiveTexture(GL_TEXTURE1);
                depthTarget->bind();
                our::Sampler::unbind(1); // Ensure no sampler is bound to unit 1 to prevent mipmap completeness issues
                postprocessMaterial->shader->set("depth_tex", 1);

                // Pass fog parameters
                postprocessMaterial->shader->set("fog_color", fogColor);
                postprocessMaterial->shader->set("fog_density", fogDensity);

                if (camera)
                {
                    // Get near and far planes from camera
                    float nearPlane = camera->near;
                    float farPlane = camera->far;

                    postprocessMaterial->shader->set("near_plane", nearPlane);
                    postprocessMaterial->shader->set("far_plane", farPlane);
                }

                glBindVertexArray(postProcessVertexArray);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(0);
            }
        }
    }
}