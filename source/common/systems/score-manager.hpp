#pragma once

#include <json/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace our
{
    // ScoreManager handles persistent storage of the player's top scores.
    // Scores are saved to a JSON file and only the best 5 are kept.
    class ScoreManager
    {
    public:
        static constexpr int MaxScores = 5;
        static constexpr const char *SavePath = "assets/data/scores.json";

        // we may change this to add some calculations

        static int computeFinalScore(int score, int coins, float currentHealth, float maxHealth)
        {

            return score + (coins * 10) + static_cast<int>(std::floor((currentHealth / maxHealth) * 100.0f));
        }

        // Load the top scores from disk.
        // Returns an empty vector if the file is missing or malformed.
        static std::vector<int> getTopScores()
        {
            std::vector<int> scores;
            std::ifstream file(SavePath);
            if (!file.is_open())
                return scores;

            try
            {
                nlohmann::json data = nlohmann::json::parse(file, nullptr, true, true);
                if (data.contains("topScores") && data["topScores"].is_array())
                {
                    for (const auto &entry : data["topScores"])
                    {
                        if (entry.is_number_integer())
                            scores.push_back(entry.get<int>());
                    }
                }
            }
            catch (const nlohmann::json::exception &e)
            {
                std::cerr << "[ScoreManager] Failed to parse " << SavePath << ": " << e.what() << std::endl;
            }

            // Ensure loaded data is sorted descending and capped
            std::sort(scores.begin(), scores.end(), std::greater<int>());
            if (scores.size() > MaxScores)
                scores.resize(MaxScores);

            return scores;
        }

        // Submit a new score: insert it into the leaderboard, keep top 5, and save.
        static void submitScore(int score)
        {
            std::vector<int> scores = getTopScores();
            scores.push_back(score);
            std::sort(scores.begin(), scores.end(), std::greater<int>());
            if (scores.size() > MaxScores)
                scores.resize(MaxScores);
            save(scores);
        }

    private:
        // Persist the score list to disk.
        static void save(const std::vector<int> &scores)
        {
            nlohmann::json data;
            data["topScores"] = scores;

            std::ofstream file(SavePath);
            if (!file.is_open())
            {
                std::cerr << "[ScoreManager] Could not open " << SavePath << " for writing." << std::endl;
                return;
            }

            file << data.dump(2) << std::endl;
        }
    };
}
