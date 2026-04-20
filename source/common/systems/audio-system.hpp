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
        ma_sound ambientWind;
        bool initialized = false;
        bool ambientPlaying = false;

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

        /**
         * Play a sound file in a continuous loop (for ambient/background audio).
         * The sound plays at reduced volume so game-event sounds layer on top.
         *
         * @param filePath  Path to the looping sound file,
         *                  e.g. "assets/sounds/sky_wind_loop.wav"
         * @param volume    Playback volume 0.0–1.0 (default 0.3)
         */
        void playLooping(const std::string &filePath, float volume = 0.3f)
        {
            if (!initialized)
            {
                std::cerr << "[AudioSystem] Cannot play looping — engine not initialized." << std::endl;
                return;
            }

            ma_result result = ma_sound_init_from_file(
                &engine, filePath.c_str(),
                MA_SOUND_FLAG_STREAM, // stream from disk to save memory
                NULL, NULL, &ambientWind);
            if (result != MA_SUCCESS)
            {
                std::cerr << "[AudioSystem] Failed to load looping sound: " << filePath
                          << " (error: " << result << ")" << std::endl;
                return;
            }

            ma_sound_set_looping(&ambientWind, MA_TRUE);
            ma_sound_set_volume(&ambientWind, volume);
            ma_sound_start(&ambientWind);
            ambientPlaying = true;

            std::cout << "[AudioSystem] Looping ambient sound started: " << filePath << std::endl;
        }

        /// Shut down the audio engine. Call during game cleanup.
        void destroy()
        {
            if (ambientPlaying)
            {
                ma_sound_stop(&ambientWind);
                ma_sound_uninit(&ambientWind);
                ambientPlaying = false;
            }
            if (initialized)
            {
                ma_engine_uninit(&engine);
                initialized = false;
                std::cout << "[AudioSystem] Audio engine destroyed." << std::endl;
            }
        }
    };
}
