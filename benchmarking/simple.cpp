#include <sipm/SiPM.h>
using namespace sipm;

int main() {
  auto prop = SiPMProperties();
  prop.setSampling(0.1);
  auto sipm = SiPMSensor(prop);

  auto rng = SiPMRandom();

  for (int i = 0; i < 1 << 8; ++i) {
    sipm.resetState();
    sipm.addPhotons(rng.randGaussian(50, 0.1, 128));
    sipm.runEvent();
  }
  sipm.resetState();
}
