#pragma once

#include <string>
#include <iostream>

// miniaudio forward declaration — the implementation lives in vendor/miniaudio/miniaudio.c
#include "miniaudio.h"

namespace our
{
    /**
     * AudioSystem wraps miniaudio's high-level engine API.
     * It provides simple fire-and-forget sound playback for game events
     * (coin pickup, tornado hit, ring pass-through, etc.).
     *
     * Usage:
     *   AudioSystem audio;
     *   audio.initialize();
     *   audio.playSound("assets/sounds/coin.wav");
     *   // ... later ...
     *   audio.destroy();
     */
    class AudioSystem
    {
        ma_engine engine;
        bool initialized = false;

    public:
        /// Initialize the audio engine. Call once during game startup.
        void initialize()
        {
            if (initialized)
                return;

            ma_engine_config config = ma_engine_config_init();
            ma_result result = ma_engine_init(&config, &engine);
            if (result != MA_SUCCESS)
            {
                std::cerr << "[AudioSystem] Failed to initialize audio engine (error: " << result << ")" << std::endl;
                return;
            }

            initialized = true;
            std::cout << "[AudioSystem] Audio engine initialized successfully." << std::endl;
        }

        /**
         * Play a sound file (fire-and-forget).
         * Supports WAV, MP3, FLAC out of the box.
         * Multiple overlapping sounds are handled automatically by miniaudio.
         *
         * @param filePath  Path to the sound file relative to the working directory,
         *                  e.g. "assets/sounds/coin.wav"
         */
        void playSound(const std::string &filePath)
        {
            if (!initialized)
            {
                std::cerr << "[AudioSystem] Cannot play sound — engine not initialized." << std::endl;
                return;
            }

            ma_result result = ma_engine_play_sound(&engine, filePath.c_str(), NULL);
            if (result != MA_SUCCESS)
            {
                std::cerr << "[AudioSystem] Failed to play sound: " << filePath
                          << " (error: " << result << ")" << std::endl;
            }
        }

        void playLooping(const std::string& filePath) {
            ma_sound_init_from_file(&engine, filePath.c_str(), 
                MA_SOUND_FLAG_STREAM,   // stream from disk, saves memory
                NULL, NULL, &ambientWind);
            ma_sound_set_looping(&ambientWind, MA_TRUE);
            ma_sound_set_volume(&ambientWind, 0.3f);  // keep it subtle
            ma_sound_start(&ambientWind);
        }
        /// Shut down the audio engine. Call during game cleanup.
        void destroy()
        {
            if (initialized)
            {
                ma_engine_uninit(&engine);
                initialized = false;
                std::cout << "[AudioSystem] Audio engine destroyed." << std::endl;
            }
        }
    };
}
