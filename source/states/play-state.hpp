#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include <systems/ring-track-system.hpp>
#include <systems/tornado-system.hpp>

// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State
{

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::RingTrackSystem ringTrack;
    our::TornadoSystem tornado;

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

        our::RingTrackConfig trackConfig;
        trackConfig.ringCount = 10;
        trackConfig.spacing = 30.0f;
        trackConfig.heightVariance = 15.0f;
        trackConfig.lateralVariance = 10.0f;
        trackConfig.ringScale = 10.0f;
        ringTrack.initialize(&world, trackConfig);

        our::TornadoConfig tornadoConfig;
        tornadoConfig.tornadoCount = 10;
        tornadoConfig.spacing = 30.0f;
        tornadoConfig.heightVariance = 15.0f;
        tornadoConfig.lateralVariance = 10.0f;
        tornadoConfig.scale = 2.0f;
        tornadoConfig.spawnChance = 0.2f;
        tornadoConfig.sideOffset = 10.0f;
        tornadoConfig.depthOffset = 10.0f;


        tornado.initialize(&world, tornadoConfig);
    }

    void onDraw(double deltaTime) override
    {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

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
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};