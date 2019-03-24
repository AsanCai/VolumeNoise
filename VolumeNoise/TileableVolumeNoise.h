#pragma once

#include <glm/gtc/noise.hpp>


/// @return Tileable Worley noise value in [0, 1].
/// @param p 3d coordinate in [0, 1], being the range of the repeatable pattern.
/// @param cellCount the number of cell for the repetitive pattern.
float getWorleyNoise(const glm::vec3& p, float cellCount);

/// @return Tileable Perlin noise value in [0, 1].
/// @param p 3d coordinate in [0, 1], being the range of the repeatable pattern.
float getPerlinNoise(const glm::vec3& p, float frequency, int octaveCount);