#pragma once
#include <cmath>

namespace our
{
    /**
     * Generates a smooth sinusoidal curve along the Z axis to offset X positions.
     * This creates a winding track path that all systems follow uniformly.
     * 
     * @param z The Z position along the track
     * @param amplitude The maximum horizontal deviation from center (default: 8.0)
     * @param frequency The tightness of the curve (default: 0.04)
     * @return The X offset to apply at this Z position
     */
    inline float trackCurveX(float z, float amplitude = 8.0f, float frequency = 0.04f)
    {
        return amplitude * std::sin(frequency * z);
    }

    /**
     * Calculates the rotation angle needed to orient an entity along the track curve.
     * Uses the derivative of the curve to determine the tangent direction.
     * 
     * @param z The Z position along the track
     * @param amplitude The maximum horizontal deviation from center (default: 8.0)
     * @param frequency The tightness of the curve (default: 0.04)
     * @return The rotation angle around the Y axis (in radians)
     */
    inline float trackCurveRotationY(float z, float amplitude = 8.0f, float frequency = 0.04f)
    {
        // Derivative: d/dz(amplitude * sin(frequency * z)) = amplitude * frequency * cos(frequency * z)
        float dxdz = amplitude * frequency * std::cos(frequency * z);
        // Calculate rotation angle from the slope
        return std::atan(dxdz);
    }
}
