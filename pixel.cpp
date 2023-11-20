#include "pixel.hpp"
#include <iostream>
#include <cstring>

/* GPIO pins used */
static const uint8_t gpios0 = 8; // this is P8.35 on beagle bone

PixelBone_Pixel::PixelBone_Pixel(uint16_t pixel_count)
    : pru0(pru_init(0)), num_pixels(pixel_count),
      buffer_size(pixel_count * sizeof(pixel_t)) {
  if (2 * buffer_size > pru0->ddr_size)
    die("Pixel data needs at least 2 * %zu, only %zu in DDR\n", buffer_size,
        pru0->ddr_size);

  ws281x = (ws281x_command_t *)pru0->data_ram;
  *(ws281x) = ws281x_command_t((unsigned)pixel_count);

  // Configure all of our output pins.
  pru_gpio(0, gpios0, 1, 0);

  // Initiate the PRU0 program
  pru_exec(pru0, "./ws281x.bin");

  // Watch for a done response that indicates a proper startup
  // TODO: timeout if it fails
  std::cout << "waiting for response from pru0... ";
  while (!ws281x->response)
    ;
  std::cout << "OK" << std::endl;
};

PixelBone_Pixel::~PixelBone_Pixel() {
  ws281x->command = 0xFF;
  pru_close(pru0);
}

void PixelBone_Pixel::show(void) {
  ws281x->pixels_dma = pru0->ddr_addr + buffer_size * current_buffer_num;

  // Wait for any current command to have been acknowledged
  while (ws281x->command)
    ;

  // Send the start command
  ws281x->command = 1;
}

void PixelBone_Pixel::moveToNextBuffer() {
  ++current_buffer_num %= 2;
};

uint32_t PixelBone_Pixel::numPixels() const { return num_pixels; }

/** Retrieve one of the two frame buffers. */
pixel_t *PixelBone_Pixel::getCurrentBuffer() const {
  return (pixel_t *)((uint8_t *)pru0->ddr + buffer_size * current_buffer_num);
}

pixel_t *PixelBone_Pixel::getPixel(uint32_t n) const {
  return &getCurrentBuffer()[n];
}

// Convert separate R,G,B into packed 32-bit RGB color.
// Packed format is always RGB, regardless of LED strand color order.
uint32_t PixelBone_Pixel::Color(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

uint32_t PixelBone_Pixel::h2rgb(uint32_t v1, uint32_t v2, uint32_t hue) {
  if (hue < 60)
    return v1 * 60 + (v2 - v1) * hue;
  if (hue < 180)
    return v2 * 60;
  if (hue < 240)
    return v1 * 60 + (v2 - v1) * (240 - hue);

  return v1 * 60;
}

/**
 * Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
 *
 * hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
 *                        120=yellow, 180=green, 240=blue, 300=violet
 *
 * saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
 *
 * lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
 */

uint32_t PixelBone_Pixel::HSL(uint32_t hue, uint32_t saturation,uint32_t lightness) {
  uint32_t red, green, blue;
  uint32_t var1, var2;
  
  if (hue > 359) hue = hue % 360;
  if (saturation > 100) saturation = 100;
  if (lightness > 100) lightness = 100;
  
  // algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
  if (saturation == 0) {
    red = green = blue = lightness * 255 / 100;
  } else {
    if (lightness < 50) {
      var2 = lightness * (100 + saturation);
    } else {
      var2 = ((lightness + saturation) * 100) - (saturation * lightness);
    }
    var1 = lightness * 200 - var2;
    red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
    green = h2rgb(var1, var2, hue) * 255 / 600000;
    blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
  }
  return (red << 16) | (green << 8) | blue;
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
uint32_t PixelBone_Pixel::getPixelColor(uint32_t n) const {
  if (n < num_pixels) {
    pixel_t *const p = getPixel(n);
    return Color(p->r, p->g, p->b);
  }
  return 0; // Pixel # is out of bounds
}

// Set pixel color from separate R,G,B components:
void PixelBone_Pixel::setPixelColor(uint32_t n, uint8_t r, uint8_t g,
                                    uint8_t b) {
  if (n < num_pixels) {
    // if(brightness) { // See notes in setBrightness()
    //   r = (r * brightness) >> 8;
    //   g = (g * brightness) >> 8;
    //   b = (b * brightness) >> 8;
    // }
    pixel_t *const p = getPixel(n);
    p->r = r;
    p->g = g;
    p->b = b;
  }
}

// Set pixel color from 'packed' 32-bit RGB color:
void PixelBone_Pixel::setPixelColor(uint32_t n, uint32_t c) {
  if (n < num_pixels) {
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;
    setPixelColor(n, r, g, b);
  }
}

void PixelBone_Pixel::setPixel(uint32_t n, pixel_t p) {
  memcpy(getPixel(n), &p, sizeof(pixel_t));
  // setPixelColor(n, p.r, p.g, p.b);
}

uint32_t PixelBone_Pixel::wait() {
  while (1) {
    uint32_t response = ws281x->response;
    if (response) {
      ws281x->response = 0;
      return response;
    }
  }
}

void PixelBone_Pixel::clear() {
  for (uint16_t i = 0; i < num_pixels; i++) {
    this->setPixelColor(i, 0, 0, 0);
  }
}
