/***************************************************************************
	Generate cloud shape and erosion texture similarly GPU Pro 7 chapter II-4
****************************************************************************/

#include <iostream>
#include <time.h>
#include <math.h>

#include "./TileableVolumeNoise.h"

#include <ppl.h>
using namespace concurrency;

#define BASE 0
#define EROSION 0

// the remap function used in the shaders as described in Gpu Pro 7. It must match when using pre packed textures
float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax) {
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

int main(int argc, char *argv[]) {

	

	// special weight for perling worley
	const float frequenceMul[6] = { 2.0f,8.0f,14.0f,20.0f,26.0f,32.0f };	

	// file stream pointer
	FILE* fp;
	// the status to open the file
	errno_t error;

#if BASE
	// Cloud base shape (will be used to generate PerlingWorley noise in he shader)
	// Note: all channels could be combined once here to reduce memory bandwith requirements.
	int cloudBaseShapeTextureSize = 128;	// !!! If this is reduce, you hsould also reduce the number of frequency in the fmb noise  !!!	
	error = fopen_s(&fp, "Perlin.cft3", "w");
	if (error != 0) {
		return -1;
	}
	/* Print header: width, height, depth */
	fprintf(fp, "%d %d %d\n", cloudBaseShapeTextureSize, cloudBaseShapeTextureSize, cloudBaseShapeTextureSize);
	const glm::vec3 normFact = glm::vec3(1.0f / float(cloudBaseShapeTextureSize));
	/* Print image */
	printf("Start printing image: Perlin.cft3 \n");
	for (int depth = 0; depth < cloudBaseShapeTextureSize; depth++) {
		printf("\b\b\b\b\b\b%3.1f %%", 100 * (float)depth / (float)cloudBaseShapeTextureSize);
		for (int height = 0; height < cloudBaseShapeTextureSize; height++) {
			for (int width = 0; width < cloudBaseShapeTextureSize; width++) {

				glm::vec3 coord = glm::vec3(width, height, depth) * normFact;

				// Perlin FBM noise
				const int octaveCount = 3;
				const float frequency = 8.0f;
				float perlinNoise = getPerlinNoise(coord, frequency, octaveCount);

				float PerlinWorleyNoise = 0.0f;
				{
					const float cellCount = 4.0f;
					const float worleyNoise0 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[0]));
					const float worleyNoise1 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[1]));
					const float worleyNoise2 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[2]));
					const float worleyNoise3 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[3]));
					const float worleyNoise4 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[4]));
					const float worleyNoise5 = (1.0f - getWorleyNoise(coord, cellCount * frequenceMul[5]));	// half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

					float worleyFBM = worleyNoise0 * 0.625f + worleyNoise1 * 0.25f + worleyNoise2 * 0.125f;

					// Perlin Worley is based on description in GPU Pro 7: Real Time Volumetric Cloudscapes.
					// However it is not clear the text and the image are matching: images does not seem to match what the result  from the description in text would give.
					// Also there are a lot of fudge factor in the code, e.g. *0.2, so it is really up to you to fine the formula you like.
					//PerlinWorleyNoise = remap(worleyFBM, 0.0, 1.0, 0.0, perlinNoise);	// Matches better what figure 4.7 (not the following up text description p.101). Maps worley between newMin as 0 and 
					PerlinWorleyNoise = remap(perlinNoise, 0.0f, 1.0f, worleyFBM, 1.0f);	// mapping perlin noise in between worley as minimum and 1.0 as maximum (as described in text of p.101 of GPU Pro 7) 
				}

				const float cellCount = 4;
				float worleyNoise0 = (1.0f - getWorleyNoise(coord, cellCount * 1));
				float worleyNoise1 = (1.0f - getWorleyNoise(coord, cellCount * 2));
				float worleyNoise2 = (1.0f - getWorleyNoise(coord, cellCount * 4));
				float worleyNoise3 = (1.0f - getWorleyNoise(coord, cellCount * 8));
				float worleyNoise4 = (1.0f - getWorleyNoise(coord, cellCount * 16));
				//float worleyNoise5 = (1.0f - Tileable3dNoise::WorleyNoise(coord, cellCount * 32));	//cellCount=2 -> half the frequency of texel, we should not go further (with cellCount = 32 and texture size = 64)

				// Three frequency of Worley FBM noise
				float worleyFBM0 = worleyNoise1 * 0.625f + worleyNoise2 * 0.25f + worleyNoise3 * 0.125f;
				float worleyFBM1 = worleyNoise2 * 0.625f + worleyNoise3 * 0.25f + worleyNoise4 * 0.125f;
				//float worleyFBM2 = worleyNoise3*0.625f + worleyNoise4*0.25f + worleyNoise5*0.125f;
				float worleyFBM2 = worleyNoise3 * 0.75f + worleyNoise4 * 0.25f; // cellCount=4 -> worleyNoise5 is just noise due to sampling frequency=texel frequency. So only take into account 2 frequencies for FBM

				int r = PerlinWorleyNoise * PerlinWorleyNoise * 255;
				int g = worleyFBM0 * 255;
				int b = worleyFBM1 * 255;
				int a = worleyFBM2 * 255;

				uint32_t pixel = 0;

				pixel |= r << 24;
				pixel |= g << 16;
				pixel |= b << 8;
				pixel |= a << 0;

				fprintf(fp, "%u ", pixel);
			}
			fprintf(fp, "\n");
		}
	}
	printf("End printing image: Perlin.cft3");
#endif

#if EROSION
	// Detail texture behing different frequency of Worley noise
	// Note: all channels could be combined once here to reduce memory bandwith requirements.
	int cloudErosionTextureSize = 32;
	error = fopen_s(&fp, "Worly.cft3", "w");
	if (error != 0) {
		return -1;
	}
	/* Print header: width, height, depth */
	fprintf(fp, "%d %d %d\n", cloudErosionTextureSize, cloudErosionTextureSize, cloudErosionTextureSize);
	printf("Start print image: Worly.cft3");
	/* Print image */
	for (int depth = 0; depth < cloudErosionTextureSize; depth++) {
		printf("\b\b\b\b\b\b%3.1f %%", 100 * (float)depth / (float)cloudErosionTextureSize);
		for (int height = 0; height < cloudErosionTextureSize; height++) {
			for (int width = 0; width < cloudErosionTextureSize; width++) {
				glm::vec3 coord = glm::vec3(width, height, depth) * normFact;

				// 3 octaves
				const float cellCount = 2;
				float worleyNoise0 = (1.0f - getWorleyNoise(coord, cellCount * 1));
				float worleyNoise1 = (1.0f - getWorleyNoise(coord, cellCount * 2));
				float worleyNoise2 = (1.0f - getWorleyNoise(coord, cellCount * 4));
				float worleyNoise3 = (1.0f - getWorleyNoise(coord, cellCount * 8));
				float worleyFBM0 = worleyNoise0 * 0.625f + worleyNoise1 * 0.25f + worleyNoise2 * 0.125f;
				float worleyFBM1 = worleyNoise1 * 0.625f + worleyNoise2 * 0.25f + worleyNoise3 * 0.125f;
				float worleyFBM2 = worleyNoise2 * 0.75f + worleyNoise3 * 0.25f; 
				// cellCount=4 -> worleyNoise4 is just noise due to sampling 
				
				int r = worleyFBM0 * 255;
				int g = worleyFBM1 * 255;
				int b = worleyFBM2 * 255;
				int a = 255;

				uint32_t pixel = 0;

				pixel |= r << 24;
				pixel |= g << 16;
				pixel |= b << 8;
				pixel |= a << 0;

				fprintf(fp, "%u ", pixel);
			}
			fprintf(fp, "\n");
		}
	}
	printf("End printing image: Worly.cft3");
#endif

	return 0;
}

