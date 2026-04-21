#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/collision.hpp>
#include <asset-loader.hpp>
#include <systems/ring-track-system.hpp>
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
// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State
{

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::CollisionSystem collisionSystem;
    our::RingTrackSystem ringTrack;
    our::TornadoSystem tornado;
    our::CoinSystem coinSystem;
    our::UIRenderSystem uiRenderer;
    our::ConeBoundarySystem coneBoundarySystem;

    void onInitialize() override
    {
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
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
        uiRenderer.initialize(getApp());

        our::RingTrackConfig trackConfig;
        if (config.contains("ringTrack"))
        {
            const auto &trackJson = config["ringTrack"];
            if (trackJson.contains("ringCount"))
                trackConfig.ringCount = trackJson["ringCount"];
            if (trackJson.contains("spacing"))
                trackConfig.spacing = trackJson["spacing"];
            if (trackJson.contains("heightVariance"))
                trackConfig.heightVariance = trackJson["heightVariance"];
            if (trackJson.contains("lateralVariance"))
                trackConfig.lateralVariance = trackJson["lateralVariance"];
            if (trackJson.contains("ringScale"))
                trackConfig.ringScale = trackJson["ringScale"];
            if (trackJson.contains("trackStartZ"))
                trackConfig.trackStartZ = trackJson["trackStartZ"];
            if (trackJson.contains("finishLineScale"))
                trackConfig.finishLineScale = trackJson["finishLineScale"];
        }
        ringTrack.initialize(&world, trackConfig);

        our::TornadoConfig tornadoConfig;
        if (config.contains("tornado"))
        {
            const auto &tornadoJson = config["tornado"];
            if (tornadoJson.contains("tornadoCount"))
                tornadoConfig.tornadoCount = tornadoJson["tornadoCount"];
            if (tornadoJson.contains("spacing"))
                tornadoConfig.spacing = tornadoJson["spacing"];
            if (tornadoJson.contains("heightVariance"))
                tornadoConfig.heightVariance = tornadoJson["heightVariance"];
            if (tornadoJson.contains("lateralVariance"))
                tornadoConfig.lateralVariance = tornadoJson["lateralVariance"];
            if (tornadoJson.contains("sideOffset"))
                tornadoConfig.sideOffset = tornadoJson["sideOffset"];
            if (tornadoJson.contains("depthOffset"))
                tornadoConfig.depthOffset = tornadoJson["depthOffset"];
            if (tornadoJson.contains("scale"))
                tornadoConfig.scale = tornadoJson["scale"];
            if (tornadoJson.contains("spawnChance"))
                tornadoConfig.spawnChance = tornadoJson["spawnChance"];
        }
        tornado.initialize(&world, tornadoConfig);

        our::ConeBoundaryConfig coneConfig;
        coneConfig.trackStartZ = 0.0f;
        coneConfig.trackEndZ = -(trackConfig.ringCount + 1) * trackConfig.spacing;

        if (config.contains("coneBoundary"))
        {
            const auto &coneJson = config["coneBoundary"];
            if (coneJson.contains("coneSpacing"))
                coneConfig.coneSpacing = coneJson["coneSpacing"];
            if (coneJson.contains("coneY"))
                coneConfig.coneY = coneJson["coneY"];
            if (coneJson.contains("scale"))
                coneConfig.scale = coneJson["scale"];
            if (coneJson.contains("margin"))
                coneConfig.coneLateralOffset = trackConfig.lateralVariance + (trackConfig.ringScale * 0.5f) + (coneJson["margin"].get<float>());
        }

        coneBoundarySystem.initialize(&world, coneConfig);
    }

    void onDraw(double deltaTime) override
    {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        coinSystem.update(&world, (float)deltaTime);
        collisionSystem.update(&world, (float)deltaTime);

        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Finally, instantly delete any marked geometry from the ECS engine so they disappear natively
        world.deleteMarkedEntities();

        // Render UI elements CHECK if a BUG appeared
        uiRenderer.render(&world, getApp());

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
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};