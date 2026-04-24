#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/collision.hpp>
#include <systems/audio-system.hpp>
#include <asset-loader.hpp>
#include <systems/ring-system.hpp>
#include <systems/track-system.hpp>
#include <systems/tornado-system.hpp>
#include <systems/cone-boundary-system.hpp>
#include <systems/coin-system.hpp>
#include <systems/ui-render-system.hpp>
#include <components/camera.hpp>
#include <components/free-camera-controller.hpp>
#include <components/health-component.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <asset-loader.hpp>
#include "systems/health-system.hpp"
// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State
{

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::CollisionSystem collisionSystem;
    our::RingSystem ringSystem;
    our::TornadoSystem tornado;
    our::CoinSystem coinSystem;
    our::UIRenderSystem uiRenderer;
    our::HealthPackSystem healthPackSystem;
    our::AudioSystem audioSystem;
    our::ConeBoundarySystem coneBoundarySystem;

    void onInitialize() override
    {
        // Initialize the audio system
        audioSystem.initialize();
        // Start ambient wind as background loop (plays underneath all other sounds)

        collisionSystem.setAudioSystem(&audioSystem);

        // First of all, we get the scene configuration from the app config
        auto &config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if (config.contains("assets"))
        {
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if (config.contains("world"))
        {
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        cameraController.setAudioSystem(&audioSystem);
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
        uiRenderer.initialize(getApp());

        our::TrackConfig trackConfig;
        if (config.contains("track"))
        {
            const auto &trackJson = config["track"];
            if (trackJson.contains("startPosition"))
            {
                auto pos = trackJson["startPosition"].get<std::vector<float>>();
                trackConfig.startPosition = glm::vec3(pos[0], pos[1], pos[2]);
            }
            if (trackJson.contains("endPosition"))
            {
                auto pos = trackJson["endPosition"].get<std::vector<float>>();
                trackConfig.endPosition = glm::vec3(pos[0], pos[1], pos[2]);
            }
            if (trackJson.contains("stagesCount"))
                trackConfig.stagesCount = trackJson["stagesCount"];
            if (trackJson.contains("innerMargin"))
                trackConfig.innerMargin = trackJson["innerMargin"];
            if (trackJson.contains("outerMargin"))
                trackConfig.outerMargin = trackJson["outerMargin"];
        }

        our::RingConfig ringConfig;
        ringConfig.trackStartPosition = trackConfig.startPosition;
        ringConfig.trackEndPosition = trackConfig.endPosition;
        ringConfig.ringsCount = trackConfig.stagesCount;
        ringConfig.margin = trackConfig.innerMargin;

        if (config.contains("rings"))
        {
            const auto &ringsJson = config["rings"];
            if (ringsJson.contains("ringScale"))
                ringConfig.ringScale = ringsJson["ringScale"];
            if (ringsJson.contains("finishLineScale"))
                ringConfig.finishLineScale = ringsJson["finishLineScale"];
        }
        std::vector<glm::vec3> ringPositions = ringSystem.initialize(&world, ringConfig);

        our::TornadoConfig tornadoConfig;
        tornadoConfig.trackStartPosition = trackConfig.startPosition;
        tornadoConfig.trackEndPosition = trackConfig.endPosition;
        tornadoConfig.tornadosCount = trackConfig.stagesCount;
        tornadoConfig.margin = trackConfig.innerMargin;

        if (config.contains("tornado"))
        {
            const auto &tornadoJson = config["tornado"];
            if (tornadoJson.contains("depthOffset"))
                tornadoConfig.depthOffset = tornadoJson["depthOffset"];
            if (tornadoJson.contains("scale"))
                tornadoConfig.scale = tornadoJson["scale"];
            if (tornadoJson.contains("spawnChance"))
                tornadoConfig.spawnChance = tornadoJson["spawnChance"];
        }
        tornado.initialize(&world, tornadoConfig);

        coinSystem.initialize(&world, ringPositions);

        our::HealthPackConfig healthConfig;
        healthConfig.spawnChance = 0.4f;
        healthConfig.minRingsBefore = 2;
        healthConfig.maxSideOffset = 6.0f;
        healthConfig.maxVertOffset = 4.0f;
        healthConfig.scale = 0.5f;

        healthPackSystem.initialize(&world, ringPositions, healthConfig);

        audioSystem.playLooping("assets/sounds/sky_wind_loop.wav", 0.3f);

        our::ConeBoundaryConfig coneConfig;
        coneConfig.startPosition = trackConfig.startPosition;
        coneConfig.endPosition = trackConfig.endPosition;

        if (config.contains("coneBoundary"))
        {
            const auto &coneJson = config["coneBoundary"];
            if (coneJson.contains("coneSpacing"))
                coneConfig.coneSpacing = coneJson["coneSpacing"];
            if (coneJson.contains("coneY"))
                coneConfig.coneY = coneJson["coneY"];
            if (coneJson.contains("scale"))
                coneConfig.scale = coneJson["scale"];
        }

        coneBoundarySystem.initialize(&world, coneConfig);
    }

    void onDraw(double deltaTime) override
    {

        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        collisionSystem.update(&world, (float)deltaTime);

        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Finally, instantly delete any marked geometry from the ECS engine so they disappear natively
        world.deleteMarkedEntities();

        // Render UI elements CHECK if a BUG appeared
        uiRenderer.render(&world, getApp());
        uiRenderer.renderDangerOverlay(getApp()->getFrameBufferSize(), collisionSystem.getDangerIntensity());

        // Get a reference to the keyboard object
        auto &keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_ESCAPE))
        {
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onDestroy() override
    {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // Destroy UI renderer
        uiRenderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Destroy audio system
        audioSystem.destroy();
        // Clear the world
        world.clear();
        coinSystem.reset();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};