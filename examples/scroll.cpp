#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cinttypes>
#include <cerrno>
//#include <unistd.h>
#include "../pixel.hpp"

int main(void) {
  const uint32_t num_pixels = 8;
  PixelBone_Pixel *const strip = new PixelBone_Pixel(num_pixels);
  while (1) {
    for (uint32_t p = 0; p < num_pixels; p++) {
      strip->clear();
      strip->setPixelColor(p, PixelBone_Pixel::Color(128, 128, 128));
      strip->show();
      usleep(50000);
    }
  }

  delete strip;
  return EXIT_SUCCESS;
}
