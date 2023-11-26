#include <Bela.h>
#include "pixel.hpp"

const uint32_t num_pixels = 8;

void pixelLoop(void*)
{
	PixelBone_Pixel strip(num_pixels);
	while(!Bela_stopRequested()) {
		for (uint32_t p = 0; p < num_pixels; p++) {
			strip.clear();
			strip.setPixelColor(p, PixelBone_Pixel::Color(128, 128, 128));
			strip.show();
			usleep(50000);
		}
	}
}

bool setup(BelaContext* context, void*)
{
	Bela_runAuxiliaryTask(pixelLoop);
	return true;
}

void render(BelaContext* context, void*)
{
}

void cleanup(BelaContext* context, void*)
{
}
