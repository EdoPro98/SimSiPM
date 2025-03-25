#include "sipm/SiPM.h"

int main(int argc, char** argv) {
  auto s = sipm::SiPMSensor();
  s.resetState();
  s.addPhoton(10);
  s.runEvent();
}
