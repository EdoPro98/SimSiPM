#ifndef SIPM_SIPMSIMULATOR_H
#define SIPM_SIPMSIMULATOR_H

#include <stdint.h>
#include <string>
#include <vector>

#include "SiPMDebugInfo.h"
#include "SiPMProperties.h"
#include "SiPMSensor.h"
namespace sipm {

class SiPMSimulator {
public:
  struct SiPMResult {
    uint32_t eventId;
    double integral, peak, tot, toa, top;
    std::vector<double> times, wavelengths;
    SiPMDebugInfo debug;
  };

  SiPMSimulator(SiPMSensor* s) : m_Sensor(s) {
    m_Intstart = 0;
    m_Intgate = s->properties().signalLength() - 1;
  }

  void readConfiguration(std::string&);

  void addEvents(const std::vector<std::vector<double>>&);
  void addEvents(const std::vector<std::vector<double>>&, const std::vector<std::vector<double>>&);
  void push_back(const std::vector<double>&);
  void push_back(const std::vector<double>&, const std::vector<double>&);

  void setSensor(SiPMSensor* s) { m_Sensor = s; }
  void setIntegration(double start, double gate, double threshold) {
    m_Intstart = start;
    m_Intgate = gate;
    m_Threshold = threshold;
  }
  void clear();

  const SiPMSensor* sensor() const { return m_Sensor; }
  SiPMSensor* sensor() { return m_Sensor; }

  void runSimulation();
  std::vector<SiPMResult> getResults() { return m_Results; }
  SiPMResult getResult(uint32_t i) { return m_Results[i]; }

private:
  std::vector<std::vector<double>> m_Times;
  std::vector<std::vector<double>> m_Wavelengths;
  std::vector<SiPMResult> m_Results;
  SiPMSensor* m_Sensor;
  uint32_t m_Nevents;
  double m_Intstart, m_Intgate, m_Threshold;
};

} // namespace sipm

#endif
