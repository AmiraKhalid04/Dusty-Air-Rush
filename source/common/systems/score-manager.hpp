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
            std::vector<int> scores = loadScoreArray("topScores");

            // Ensure loaded data is sorted descending and capped
            std::sort(scores.begin(), scores.end(), std::greater<int>());
            if (scores.size() > MaxScores)
                scores.resize(MaxScores);

            return scores;
        }

        // Load the most recent winning scores from disk.
        // The newest win is returned first.
        static std::vector<int> getRecentWins()
        {
            std::vector<int> scores = loadScoreArray("recentWins");
            if (scores.empty())
                scores = loadScoreArray("topScores");
            if (scores.size() > MaxScores)
                scores.resize(MaxScores);
            return scores;
        }

        // Submit a new winning score: keep both the top 5 and the last 5 wins.
        static void submitWinScore(int score)
        {
            std::vector<int> topScores = getTopScores();
            topScores.push_back(score);
            std::sort(topScores.begin(), topScores.end(), std::greater<int>());
            if (topScores.size() > MaxScores)
                topScores.resize(MaxScores);

            std::vector<int> recentWins = getRecentWins();
            recentWins.insert(recentWins.begin(), score);
            if (recentWins.size() > MaxScores)
                recentWins.resize(MaxScores);

            save(topScores, recentWins);
        }

        // Backward-compatible alias for existing callers.
        static void submitScore(int score)
        {
            submitWinScore(score);
        }

    private:
        static nlohmann::json loadData()
        {
            std::ifstream file(SavePath);
            if (!file.is_open())
                return nlohmann::json::object();

            try
            {
                return nlohmann::json::parse(file, nullptr, true, true);
            }
            catch (const nlohmann::json::exception &e)
            {
                std::cerr << "[ScoreManager] Failed to parse " << SavePath << ": " << e.what() << std::endl;
                return nlohmann::json::object();
            }
        }

        static std::vector<int> loadScoreArray(const char *field)
        {
            std::vector<int> scores;
            nlohmann::json data = loadData();
            if (data.contains(field) && data[field].is_array())
            {
                for (const auto &entry : data[field])
                {
                    if (entry.is_number_integer())
                        scores.push_back(entry.get<int>());
                }
            }
            return scores;
        }

        // Persist both score views to disk.
        static void save(const std::vector<int> &topScores, const std::vector<int> &recentWins)
        {
            nlohmann::json data;
            data["topScores"] = topScores;
            data["recentWins"] = recentWins;

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
