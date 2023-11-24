#include <ola/Logging.h>
#include <ola/OlaClientWrapper.h>
#include "../pixel.hpp"

static const unsigned int UNIVERSE = 1;
static const int NUM_PIXELS = 8;
static PixelBone_Pixel* strip;



void NewDmx(const ola::client::DMXMetadata &metadata,
            const ola::DmxBuffer &data) {
  //OLA_INFO << "Received " << (int) data.Size();
  //OLA_INFO << "Channel 1: " << (int) data.Get(0);
  //int r = (int) data.Get(0);
  //int g = (int) data.Get(1);
  //int b = (int) data.Get(2);

  strip->clear();
  for(int i=0; i<NUM_PIXELS; i++) {
    int r = (int) data.Get(i*3);
    int g = (int) data.Get(i*3+1);
    int b = (int) data.Get(i*3+2);
    strip->setPixelColor(i, PixelBone_Pixel::Color(r, g, b) );
  }
  strip->show();
}

void RegisterComplete(const ola::client::Result &result) {
  if (!result.Success()) {
    OLA_WARN << "Register failed: " << result.Error();
  } else {
    OLA_INFO << "Register completed";
  }
}


int main() {
 strip = new PixelBone_Pixel(NUM_PIXELS); 
 ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);
  ola::client::OlaClientWrapper wrapper;

  if (!wrapper.Setup())
    return -1;

  // Get the underlying OlaCallbackClient object
  ola::client::OlaClient *client = wrapper.GetClient();
  // Set the observer and register our interest in this universe
  client->SetDMXCallback( ola::NewCallback(&NewDmx) );
  client->RegisterUniverse(
			UNIVERSE, 
			ola::client::REGISTER, 
			ola::NewSingleCallback(&RegisterComplete)
			);
  wrapper.GetSelectServer()->Run();
  delete(strip);
  return EXIT_SUCCESS;
}
