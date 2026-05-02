#pragma once

#include <json/json.hpp>
#include <glm/vec3.hpp>

namespace our
{
    enum class GameplayTheme
    {
        Classic,
        Girlish
    };

    inline GameplayTheme &activeGameplayThemeStorage()
    {
        static GameplayTheme activeTheme = GameplayTheme::Classic;
        return activeTheme;
    }

    inline void setGameplayTheme(GameplayTheme theme)
    {
        activeGameplayThemeStorage() = theme;
    }

    inline GameplayTheme getGameplayTheme()
    {
        return activeGameplayThemeStorage();
    }

    inline bool isGirlishTheme(GameplayTheme theme = getGameplayTheme())
    {
        return theme == GameplayTheme::Girlish;
    }

    inline const char *getCollectibleEmoji(GameplayTheme theme = getGameplayTheme())
    {
        return isGirlishTheme(theme) ? u8"🎀" : u8"🪙";
    }

    inline const char *getCollectibleSummaryLabel(GameplayTheme theme = getGameplayTheme())
    {
        return isGirlishTheme(theme) ? "Bows Collected" : "Coins Collected";
    }

    inline float getCollectibleScaleMultiplier(GameplayTheme theme = getGameplayTheme())
    {
        return isGirlishTheme(theme) ? 0.08f : 1.0f;
    }

    inline glm::vec3 getCollectibleColliderCenter(GameplayTheme theme = getGameplayTheme())
    {
        return isGirlishTheme(theme) ? glm::vec3(0.0f) : glm::vec3(0.0f, 0.8f, 0.0f);
    }

    inline float getCollectibleRadius(GameplayTheme theme = getGameplayTheme())
    {
        return isGirlishTheme(theme) ? 0.55f : 0.4f;
    }

    inline void applyGameplayThemeAssets(nlohmann::json &assets, GameplayTheme theme = getGameplayTheme())
    {
        if (!assets.is_object())
            return;

        if (!isGirlishTheme(theme))
            return;

        if (assets.contains("textures") && assets["textures"].is_object())
        {
            assets["textures"]["dusty-texture"] = "assets/textures/dusty-pink.png";
            assets["textures"]["coin-texture"] = "assets/textures/pink_bow.png";
        }

        if (assets.contains("meshes") && assets["meshes"].is_object())
        {
            assets["meshes"]["coin"] = "assets/textures/pink_bow.obj";
        }

        if (assets.contains("materials") && assets["materials"].is_object())
        {
            auto &coinMaterial = assets["materials"]["coin"];
            if (coinMaterial.is_object())
            {
                coinMaterial["alphaThreshold"] = 0.05f;
            }
        }
    }
}
