#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../components/light.hpp"
#include "../deserialize-utils.hpp"
#include <GLFW/glfw3.h>

namespace our
{

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json &config)
    {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        if (config.contains("sky_lighting"))
        {
            auto skyLight = config["sky_lighting"];
            skyTop = skyLight.value("top", glm::vec3(1.0f));
            skyHorizon = skyLight.value("horizon", glm::vec3(1.0f));
            skyBottom = skyLight.value("bottom", glm::vec3(1.0f));
        }

        if (config.contains("sky"))
        {
            // The sky shader is a ShaderToy-style fullscreen shader.
            // We use fullscreen.vert so gl_FragCoord covers the whole screen,
            // and we draw it as a fullscreen triangle (no sphere mesh needed).
            ShaderProgram *skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/sky.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            // Draw the sky fullscreen before opaque geometry.
            // Disable depth write and depth test so it acts as a background.
            PipelineState skyPipelineState{};
            skyPipelineState.depthTesting.enabled = false;
            skyPipelineState.faceCulling.enabled = false;
            skyPipelineState.blending.enabled = false;
            skyPipelineState.colorMask = {true, true, true, true};
            skyPipelineState.depthMask = false;

            // Use a plain TexturedMaterial as the container (no actual texture needed).
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;

            // Create a VAO for the fullscreen triangle draw call.
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
        }
    }

    void ForwardRenderer::destroy()
    {
        // Delete all objects related to the sky
        if (skyMaterial)
        {
            glDeleteVertexArrays(1, &skyVertexArray);
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

        for (auto entity : world->getEntities())
        {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera)
                camera = entity->getComponent<CameraComponent>();

            // If this entity has a light component
            if (auto light = entity->getComponent<LightComponent>(); light)
            {
                activeLights.push_back({light, entity->getLocalToWorldMatrix()});
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
                if (debugMat)
                {
                    RenderCommand command;
                    glm::mat4 baseTransform = glm::translate(entity->getLocalToWorldMatrix(), collider->center);

                    if (collider->shapeType == ColliderType::Sphere)
                    {
                        command.localToWorld = glm::scale(baseTransform, glm::vec3(collider->sphereRadius));
                        command.mesh = our::AssetLoader<Mesh>::get("sphere");
                    }
                    else
                    {
                        command.localToWorld = glm::scale(baseTransform, collider->aabbExtents);
                        command.mesh = our::AssetLoader<Mesh>::get("cube");
                    }

                    command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                    command.material = debugMat;

                    // Wireframes look better in transparent pass to overlay gracefully
                    transparentCommands.push_back(command);
                }
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

        glm::vec3 cameraPosition = glm::vec3(camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));

        // Draw the sky as a fullscreen pass FIRST so opaque/transparent objects render on top
        if (this->skyMaterial)
        {
            this->skyMaterial->setup();

            // Only the camera's VIEW DIRECTION matters for the sky — the sky shader is a
            // finite procedural scene and passing the actual game-world position causes the
            // raymarcher to leave its terrain bounds, producing solid-black areas.
            // We lock `eye` to a fixed vantage point inside the sky world and only
            // forward the camera's normalized look direction so the sky rotates with the player.
            auto skyM = camera->getOwner()->getLocalToWorldMatrix();
            glm::vec3 skyFwd = glm::normalize(glm::vec3(skyM * glm::vec4(0, 0, -1, 0)));

            // Fixed position: 200 units above sea level, near the origin of the sky world.
            // This ensures the raymarcher always sees terrain, ocean, and clouds.
            glm::vec3 skyEye = glm::vec3(0.0f, 200.0f, 0.0f);

            // Set uniforms for the ShaderToy-style sky shader
            this->skyMaterial->shader->set("iTime", (float)glfwGetTime());
            this->skyMaterial->shader->set("iResolution", glm::vec2(windowSize));
            this->skyMaterial->shader->set("eye", skyEye);
            this->skyMaterial->shader->set("forward", skyFwd);

            // Draw the fullscreen triangle (fullscreen.vert handles positions, no VAO data needed)
            glBindVertexArray(skyVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);

            // Re-enable depth mask for opaque rendering
            glDepthMask(GL_TRUE);
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

                command.material->shader->set("light_count", (int)activeLights.size());
                for (int i = 0; i < (int)activeLights.size(); i++)
                {
                    std::string prefix = "lights[" + std::to_string(i) + "].";
                    command.material->shader->set(prefix + "type", (int)activeLights[i].light->lightType);
                    command.material->shader->set(prefix + "color", activeLights[i].light->diffuse);
                    command.material->shader->set(prefix + "attenuation", activeLights[i].light->attenuation);
                    glm::vec3 pos = activeLights[i].localToWorld * glm::vec4(0, 0, 0, 1);
                    glm::vec3 dir = activeLights[i].localToWorld * glm::vec4(0, 0, -1, 0);
                    command.material->shader->set(prefix + "position", pos);
                    command.material->shader->set(prefix + "direction", glm::normalize(dir));
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

                command.material->shader->set("light_count", (int)activeLights.size());
                for (int i = 0; i < (int)activeLights.size(); i++)
                {
                    std::string prefix = "lights[" + std::to_string(i) + "].";
                    command.material->shader->set(prefix + "type", (int)activeLights[i].light->lightType);
                    command.material->shader->set(prefix + "color", activeLights[i].light->diffuse);
                    command.material->shader->set(prefix + "attenuation", activeLights[i].light->attenuation);
                    glm::vec3 pos = activeLights[i].localToWorld * glm::vec4(0, 0, 0, 1);
                    glm::vec3 dir = activeLights[i].localToWorld * glm::vec4(0, 0, -1, 0);
                    command.material->shader->set(prefix + "position", pos);
                    command.material->shader->set(prefix + "direction", glm::normalize(dir));
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
            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }
    }

}