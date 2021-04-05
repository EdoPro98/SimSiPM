#include "SiPMSimulator.h"
#include "SiPMAnalogSignal.h"
#include "SiPMDebugInfo.h"
#include <stdint.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <iostream>

namespace sipm {
void SiPMSimulator::addEvents(const std::vector<std::vector<double>>& t) {
  m_Times = t;
  m_Nevents = t.size();
}


void SiPMSimulator::addEvents(const std::vector<std::vector<double>>& t, const std::vector<std::vector<double>>& w) {
  m_Times = t;
  m_Wavelengths = w;
  m_Nevents = t.size();
}


void SiPMSimulator::push_back(const std::vector<double>& t) {
  m_Times.emplace_back(t);
  m_Nevents++;
}


void SiPMSimulator::push_back(const std::vector<double>& t, const std::vector<double>& w) {
  m_Times.emplace_back(t);
  m_Wavelengths.emplace_back(w);
  m_Nevents++;
}


void SiPMSimulator::clear() {
  m_Times.clear();
  m_Wavelengths.clear();
  m_Results.clear();
  m_Nevents = 0;
}

#ifdef _OPENMP
void SiPMSimulator::runSimulation() {
  uint32_t nThreads = omp_get_max_threads();
  omp_set_num_threads(nThreads);

  SiPMSensor p_Sensors[nThreads];
  for (uint32_t i = 0; i < nThreads; ++i) {
    p_Sensors[i].setProperties(m_Sensor->properties());
  }

  bool needWlen = false;
  bool hasWlen = false;
  if (m_Sensor->properties().pdeType() == SiPMProperties::PdeType::kSpectrumPde) {
    needWlen = true;
  }
  if (m_Wavelengths.size() != 0) {
    hasWlen = true;
  }

  if (needWlen == false) {
#pragma omp parallel for
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      const uint32_t n_thread = omp_get_thread_num();
      p_Sensors[n_thread].resetState();
      p_Sensors[n_thread].addPhotons(m_Times[i]);
      p_Sensors[n_thread].runEvent();

      SiPMAnalogSignal l_Signal = p_Sensors[n_thread].signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.idx = i;
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
#pragma omp critical
      { m_Results.push_back(l_Result); }
    }
  }
  if (needWlen == true && hasWlen == true) {
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      m_Sensor->resetState();
      m_Sensor->addPhotons(m_Times[i], m_Wavelengths[i]);
      m_Sensor->runEvent();

      SiPMAnalogSignal l_Signal = m_Sensor->signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.wavelengths = m_Wavelengths[i];
      l_Result.idx = i;
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
      m_Results.push_back(l_Result);
    }
  }
  if (needWlen == true && hasWlen == false) {
    m_Sensor->properties().setPdeType(SiPMProperties::PdeType::kNoPde);
    std::cerr << "Running simulation without PDE! Missing wavelengths..." << std::endl;
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      m_Sensor->resetState();
      m_Sensor->addPhotons(m_Times[i]);
      m_Sensor->runEvent();

      SiPMAnalogSignal l_Signal = m_Sensor->signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.idx = i;
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
      m_Results.push_back(l_Result);
    }
  }
}
#else
void SiPMSimulator::runSimulation() {
  bool needWlen = false;
  bool hasWlen = false;
  if (m_Sensor->properties().pdeType() == SiPMProperties::PdeType::kSpectrumPde) {
    needWlen = true;
  }
  if (m_Wavelengths.size() != 0) {
    hasWlen = true;
  }

  if (needWlen == false) {
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      m_Sensor->resetState();
      m_Sensor->addPhotons(m_Times[i]);
      m_Sensor->runEvent();

      SiPMAnalogSignal l_Signal = m_Sensor->signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.idx = i;
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
      m_Results.push_back(l_Result);
    }
  }
  if (needWlen == true && hasWlen == true) {
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      m_Sensor->resetState();
      m_Sensor->addPhotons(m_Times[i], m_Wavelengths[i]);
      m_Sensor->runEvent();

      SiPMAnalogSignal l_Signal = m_Sensor->signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.wavelengths = m_Wavelengths[i];
      l_Result.idx = i;
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
      m_Results.push_back(l_Result);
    }
  }
  if (needWlen == true && hasWlen == false) {
    m_Sensor->properties().setPdeType(SiPMProperties::PdeType::kNoPde);
    std::cerr << "Running simulation without PDE! Missing wavelengths..." << std::endl;
    for (uint32_t i = 0; i < m_Nevents; ++i) {
      m_Sensor->resetState();
      m_Sensor->addPhotons(m_Times[i]);
      m_Sensor->runEvent();

      SiPMAnalogSignal l_Signal = m_Sensor->signal();
      SiPMResult l_Result;

      l_Result.times = m_Times[i];
      l_Result.idx = i;
      l_Result.integral = l_Signal.integral(m_Intstart, m_Intgate, 0.5);
      l_Result.peak = l_Signal.peak(m_Intstart, m_Intgate, 0.5);
      l_Result.tot = l_Signal.tot(m_Intstart, m_Intgate, 0.5);
      l_Result.toa = l_Signal.toa(m_Intstart, m_Intgate, 0.5);
      l_Result.top = l_Signal.top(m_Intstart, m_Intgate, 0.5);
      m_Results.push_back(l_Result);
    }
  }
}
#endif


}  // namespace sipm
