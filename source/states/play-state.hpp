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
#include <systems/world-boundary-system.hpp>
#include <systems/tornado-system.hpp>
#include <systems/runway-light-system.hpp>
#include <systems/coin-system.hpp>
#include <systems/ui-render-system.hpp>
#include <systems/text-popup-system.hpp>
#include <components/camera.hpp>
#include <components/free-camera-controller.hpp>
#include <components/dusty.hpp>
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
    our::TornadoSystem tornadoSystem;
    our::CoinSystem coinSystem;
    our::UIRenderSystem uiRenderer;
    our::HealthPackSystem healthPackSystem;
    our::AudioSystem audioSystem;
    our::TextPopupSystem textPopupSystem;
    float lastDeltaTime = 0.0f;
    our::RunwayLightSystem runwayLightSystem;
    our::WorldBoundarySystem worldBoundarySystem;

    float playTime = 0.0f;
    bool planeFlapping = true;

    void onInitialize() override
    {
        playTime = 0.0f;
        // Initialize the audio system
        audioSystem.initialize();
        // Start ambient wind as background loop (plays underneath all other sounds)

        collisionSystem.setAudioSystem(&audioSystem);
        collisionSystem.setTextPopupSystem(&textPopupSystem);

        // First of all, we get the scene configuration from the app config
        auto &config = getApp()->getConfig()["scene"];

        // Read plane flapping toggle
        if (config.contains("plane_flapping"))
            planeFlapping = config["plane_flapping"];
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
        worldBoundarySystem.initialize(trackConfig);

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

        // Assign the generated logic variables (total track rings) to the player's dusty tracker
        for (auto entity : world.getEntities())
        {
            if (auto dusty = entity->getComponent<our::DustyComponent>())
            {
                dusty->totalRings = trackConfig.stagesCount;
                break;
            }
        }

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
            if (tornadoJson.contains("angularVelocity"))
                tornadoConfig.angularVelocity = tornadoJson["angularVelocity"];
            if (tornadoJson.contains("moveSpeedXMin"))
                tornadoConfig.moveSpeedXMin = tornadoJson["moveSpeedXMin"];
            if (tornadoJson.contains("moveSpeedXMax"))
                tornadoConfig.moveSpeedXMax = tornadoJson["moveSpeedXMax"];
        }
        tornadoSystem.initialize(&world, tornadoConfig);

        coinSystem.initialize(&world, ringPositions);

        our::HealthPackConfig healthConfig;
        healthConfig.spawnChance = 0.4f;
        healthConfig.minRingsBefore = 2;
        healthConfig.maxSideOffset = 6.0f;
        healthConfig.maxVertOffset = 4.0f;
        healthConfig.scale = 0.5f;

        healthPackSystem.initialize(&world, ringPositions, healthConfig);

        audioSystem.playLooping("assets/sounds/sky_wind_loop.wav", 0.3f);

        our::RunwayLightConfig runwayLightConfig;
        runwayLightConfig.startPosition = trackConfig.startPosition;
        runwayLightConfig.endPosition = trackConfig.endPosition;

        if (config.contains("runwayLights"))
        {
            const auto &runwayLightsJson = config["runwayLights"];
            if (runwayLightsJson.contains("spacing"))
                runwayLightConfig.spacing = runwayLightsJson["spacing"];
            if (runwayLightsJson.contains("scale"))
                runwayLightConfig.scale = runwayLightsJson["scale"];
        }

        runwayLightSystem.initialize(&world, runwayLightConfig);
    }

    void onDraw(double deltaTime) override
    {
        lastDeltaTime = (float)deltaTime;
        playTime += (float)deltaTime;

        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        collisionSystem.update(&world, (float)deltaTime);
        tornadoSystem.update(&world, (float)deltaTime);

        textPopupSystem.update((float)deltaTime);
        worldBoundarySystem.update(&world);
        // Animate wings (flapping)
        if (planeFlapping)
        {
            float flapAngle = std::sin(playTime * 15.0f) * glm::radians(25.0f);
            for (auto entity : world.getEntities())
            {
                if (entity->name.find("Left Wing") != std::string::npos)
                {
                    entity->localTransform.rotation.y = flapAngle;
                }
                else if (entity->name.find("Right Wing") != std::string::npos)
                {
                    entity->localTransform.rotation.y = -flapAngle;
                }
            }
        }

        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Finally, instantly delete any marked geometry from the ECS engine so they disappear natively
        world.deleteMarkedEntities();

        // Render UI elements CHECK if a BUG appeared
        uiRenderer.render(&world, getApp());
        uiRenderer.renderDangerOverlay(getApp()->getFrameBufferSize(), collisionSystem.getDangerIntensity());
        uiRenderer.renderBoundaryFlash(getApp()->getFrameBufferSize(), worldBoundarySystem.getFlashIntensity());

        // Get a reference to the keyboard object
        auto &keyboard = getApp()->getKeyboard();

        if (keyboard.justPressed(GLFW_KEY_ESCAPE))
        {
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }

        // Check win/loss state from Player's dusty component
        for (auto entity : world.getEntities())
        {
            if (auto dusty = entity->getComponent<our::DustyComponent>())
            {
                if (dusty->isDead)
                {
                    std::cout << "\n=============================================" << std::endl;
                    std::cout << "          MISSION FAILED - HEALTH DEPLETED    " << std::endl;
                    std::cout << " Rings Passed: " << dusty->score << " / " << dusty->totalRings << std::endl;
                    std::cout << " Coins Collected: " << dusty->coins << std::endl;
                    std::cout << " Time Survived: " << playTime << "s" << std::endl;
                    std::cout << "=============================================\n"
                              << std::endl;
                    getApp()->changeState("loss");
                }
                else if (dusty->isWon)
                {
                    std::cout << "\n=============================================" << std::endl;
                    std::cout << "            MISSION COMPLETE!                 " << std::endl;
                    std::cout << " Rings Passed: " << dusty->score << " / " << dusty->totalRings << std::endl;
                    std::cout << " Coins Collected: " << dusty->coins << std::endl;
                    std::cout << " Final Health: " << dusty->currentHealth << " / " << dusty->maxHealth << std::endl;
                    std::cout << " Time: " << playTime << "s" << std::endl;
                    std::cout << "=============================================\n"
                              << std::endl;
                    getApp()->changeState("win");
                }
                break; // Dusty component handled
            }
        }
    }

    void onImmediateGui() override
    {
        textPopupSystem.render();
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
        textPopupSystem.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};