#pragma once
#include <cmath>

namespace our
{
    struct TrackCurveConfig
    {
        float amplitude = 8.0f;   // Maximum horizontal deviation from center
        float frequency = 0.04f;  // Tightness of the curve
    };

    // Global track curve configuration
    static TrackCurveConfig g_trackCurveConfig;

    /**
     * Sets the global track curve configuration.
     * Call this during initialization to load values from config.
     */
    inline void setTrackCurveConfig(const TrackCurveConfig& config)
    {
        g_trackCurveConfig = config;
    }

    /**
     * Generates a smooth sinusoidal curve along the Z axis to offset X positions.
     * This creates a winding track path that all systems follow uniformly.
     * 
     * @param z The Z position along the track
     * @return The X offset to apply at this Z position
     */
    inline float trackCurveX(float z)
    {
        return g_trackCurveConfig.amplitude * std::sin(g_trackCurveConfig.frequency * z);
    }

    /**
     * Calculates the rotation angle needed to orient an entity along the track curve.
     * Uses the derivative of the curve to determine the tangent direction.
     * 
     * @param z The Z position along the track
     * @return The rotation angle around the Y axis (in radians)
     */
    inline float trackCurveRotationY(float z)
    {
        // Derivative: d/dz(amplitude * sin(frequency * z)) = amplitude * frequency * cos(frequency * z)
        float dxdz = g_trackCurveConfig.amplitude * g_trackCurveConfig.frequency * std::cos(g_trackCurveConfig.frequency * z);
        // Calculate rotation angle from the slope
        return std::atan(dxdz);
    }
}
