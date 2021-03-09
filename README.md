# SimSiPM
![GitHub release](https://img.shields.io/github/v/release/EdoPro98/SimSiPM?include_prereleases)

<p align="center"><img src="/images/signals.svg" width=500></p>

![GCC](https://github.com/EdoPro98/SimSiPM/workflows/GCC/badge.svg?branch=master&event=push)
![AppleClang](https://github.com/EdoPro98/SimSiPM/workflows/AppleClang/badge.svg?branch=master&event=push)

![GitHub issues](https://img.shields.io/github/issues/EdoPro98/SimSiPM)
![GitHub last commit](https://img.shields.io/github/last-commit/EdoPro98/SimSiPM)
[![GitHub license](https://img.shields.io/github/license/EdoPro98/SimSiPM)](https://github.com/EdoPro98/SimSiPM/blob/master/LICENSE)

[![Downloads](https://static.pepy.tech/personalized-badge/sipm?period=total&units=international_system&left_color=grey&right_color=blue&left_text=PyPi%20Downloads)](https://pepy.tech/project/sipm)
[![Downloads](https://static.pepy.tech/personalized-badge/sipm?period=week&units=international_system&left_color=grey&right_color=blue&left_text=PyPi%20Downloads%20/%20Week)](https://pepy.tech/project/sipm)

## Table of contents
1. [Introduction](#introduction)  
2. [Features](#features)  
3. [Installation](#installation)
  - [C++](#c++install)
  - [Python](#pyinstall)
4. [C++ Basic use](#c++_basic_bsage)  
5. [Python Basic use](#python_basic_usage)  
6. [Advanced use](#advanced_use)  
  - [Pde](#pde)
  - [Hit distribution](#hit)
7. [Contributing](#contrib)

## <a name="introduction"></a>Introduction
SimSiPM is a C++ library providing a set of object-oriented tools with all the functionality needed to describe and simulate Silicon PhotonMultipliers (SiPM) sensors.
It can be used to simulate SiPM signals in order to have a detailed description of a detector or it can be used to investigate how different SiPM parameters affect the detector.

SimSiPM has beed developed followind FCCSW C++ rules and guidelines and it is focused on SiPM simulation for high-energy physics and particle physics experiments however it can be used to simulate any kind of experiment involving SiPM devices.

SimSiPM does not have any external dependancy making it the perfect candidate to be used in an already existing environment (Geant4 or DD4HEP) or as "stand-alone".

## <a name="features"></a>Features
- Easy to use:
  - Straight forward installation without external dependancies
  - Easy to use OOP paradigm
  - Python implementation
- Description of SiPM sensors:
  - Based on datasheet values or measurable quantities
  - High level of customization allowing to describe a wide range of use cases
- High performance:
  - Fast signal generation
  - Low memory footprint


## <a name="installation"></a>Installation
SimSiPM has not external dependancies other than CMake and optionally Pybind11.

### <a name="c++install"></a>C++
SimSiPM can be installed using the standard CMake workflow:
```sh
# In SimSiPM directory
cmake -B build -S .
make -C build
make -C build install
```
Installation directory can be specified with `-DCMAKE_INSTALL_PREFIX` variable.  
Python bindings can be installed in the default python site-packages path by adding the variable `-DCOMPILE_PYTHON_BINDINGS=ON` but this requires Pybind11 to be installed.

### <a name="pyinstall"></a>Python
It is also possible to install only the python version via pip but performance might not be as good as the source code version:
```sh
pip install SiPM
```

## <a name="C++_basic_usage"></a>C++ basic use

### SiPMProperties
SiPMProperties object stores SiPM parameters
```cpp
#include "SiPMProperties.h"
using namespace sipm;

// Create a SiPMProperties object
SiPMProperties myProperties;

// Edit some parameters
myProperties.setDcr(250e3);           // Using proper setter
myProperties.setPropery("Xt",0.03);   // Using parameter name
```

### SiPMSensor
SiPMSensor object is used to generate signals
```cpp
#include "SiPMProperties.h"
using namespace sipm;

// Create a SiPMSensor object
SiPMSensor mySensor(myProperties);

// Change parameters
mySensor.properties().setAp(0.01);    // Using proper getter/setter
mySensor.setProperty("Pitch", 25);    // Using parameter name
```

### Input and simulation
Input of the simulation is either the arriving time of a photon on the SiPM surface or both the arriving time of the photon and its wavelength.

It is possible to add individual photons in a loop
```cpp
mySensor.resetState();
for(...){
  // Generate times for photons
  mySensor.addPhoton(time);    // Appends a single photon (time is in ns)
}
mySensor.runEvent();          // Runs the simulation
```

It is also possible to add all photons at once
```cpp
std::vector<double> times = {13.12, 25.45, 33.68};
mySensor.resetState();
mySensor.addPhotons(times);    // Sets photon times (times are in ns) (not appending)
mySensor.runEvent();          // Runs the simulation
```

### Signal output and signal features
After running the simulation the signal can be retrived:
```cpp
SiPMAnalogSignal mySignal = mySensor.signal();

double integral = signal.integral(5,250,0.5);   // (intStart, intGate, threshold)
double peak = signal.peak(5,250,0.5);   // (intStart, intGate, threshold)
double toa = signal.toa(5,250,0.5);   // (intStart, intGate, threshold)
double tot = signal.tot(5,250,0.5);   // (intStart, intGate, threshold)

// It is possible to iterate throwg an analog signal
for(int i=0;i<mySignal.size();++i){
  // Do something with mySignal[i]
}

// It is possible to convert an analog signal to a simple vector
std::vector<double> waveform = mySignal.waveform();
```

### Complete event loop
A typical event loop would look like:
```cpp
// Create sensor and set parameters
SiPMProperties myProperties;
SiPMSensor mySensor(myProperties);
// ...

// Store results in here
std::vector<double> integral(NEVENTS);
// peak
// ...

for(int i=0;i<NEVENTS;++i){
  // Generate photons times accordingly
  // to your experimental setup
  mySensor.resetState();
  mySensor.addPhotons(times);
  mySensor.runEvent();

  SiPMAnalogSignal mySignal = mySensor.signal();

  integral[i] = signal.integral(10,250,0.5);
  // peak
  // ...
}
```
## <a name="python_basic_usage"></a>Python basic use
Python bindings are generated using Pybind11 so the usage is very similar to C++ but with python syntax.

```python
from SiPM import SiPMSensor, SiPMProperties

myProperties = SiPMProperties()
myProperties.setDcr(250e3)
myProperties.seProperty("Xt",0.03)

mySensor = SiPMSensor(myProperties)

mySensor.resetState()
mySensor.addPhotons([13.12, 25.45, 33.68])
mySensor.runEvent()

mySignal = mySensor.signal()
integral = mySignal.integral(10,250,0.5)
```
## <a name="adv"></a>Advanced use
### <a name="pde"></a>PDE
#### No Pde
Tracking a large number of photons is a very heavy task and since most of photons will not be detected due to photon detection efficiency (PDE) it would be a waste of time.

By default SiPM sensors have PDE set to 100% so every photon is converted to a photoelectron and is detected. In this way it is possible to calculate photon statistic ahead and track only the photons that will be detected.

#### Simple PDE
It is possible to account for PDE in the simulation using a fixed value of PDE for all photons. In this case the probability to detect a photon is proportional to PDE.
```cpp
// Set in SiPMProperties
myProperties.setPdeType(sipm::SiPMProperties::PdeType::kSimplePde);
myProperties.setPde(0.27);

// Change setting of a sensor
mySensor.properties().setPdeType(sipm::SiPMProperties::PdeType::kSimplePde);
mySensor.setProperty("Pde",0.27); // or mySensor.properties().setPde(0.27);
```
To revert back at default setting of 100% PDE use `setPdeType(sipm::SiPMProperties::PdeType::kSimplePde)`

#### Spectral PDE
In most SiPM sensors PDE strongly depends on photon wavelength. In some cases it might be necessary to consider the spectral response of the SiPM for a more accurate simulation.
This can be done by feeding the SiPM settings with two arrays containing photon wavelengths and corresponding PDEs.

In this case it is also necessary to input photon wavelength along with its time.
```cpp
std::vector<double> wlen = {300, 400, 500, 600, 700, 800};
std::vector<double> pde  = {0.01, 0.20, 0.33, 0.27, 0.15, 0.05};

myProperties.setPdeType(sipm::SiPMProperties::PdeType::kSpectrumPde);
myProperties.setPdeSpectrum(wlen,pde);

// or using a std::map
// std::map<double,double> wlen_pde = {{300, 0.01}, {400, 0.20}, {500, 0.33}, ...};
// myProperties.setPdeSpectrum(wlen_pde);

// Adding photons to the sensor
mySensor.addPhoton(photonTime, photonWlen);
// or mySensor.addPhotons(photonTimes, photonWlens);
```
<p align="center"><img src="/images/pde.png" width=500></p>

The values inserted by the user are linearly interpolated to calculate the PDE for each wavelength so it is better to add a reasonable number of values.

### <a name="hit"></a>Hit distribution
By default photoelectrons are distributed uniformly on the surface of the SiPM. In most cases this assumption resembles what happens in a typical setup but sometimes the geometry and optical characteristics of the setup lead to an unheaven distribution of the light on the sensor's surface.
#### Uniform hit distribution
This is the default setting. Each SiPM cell has the same probability to be hitted.
```cpp
myPropertie.setHitDistribution(sipm::SiPMProperties::HitDistribution::kUniform);
```

#### Circular hit distribution
In this case 95% of photons are placed in a circle centered in the sensor and with a diameter that is the same as the sensor's side lenght. The remaining 5% is distributed uniformly on the sensor.
<p align="center"><img src="/images/circleHits.png" width=250></p>

```cpp
myPropertie.setHitDistribution(sipm::SiPMProperties::HitDistribution::kCircle);
```

#### Gaussian hit distribution
In this case 95% of the photons are distributed following a gaussian distribution centered in the sensor. The remaining 5% is distributed uniformly on the sensor.  
<p align="center"><img src="/images/gaussianHits.png" width=250></p>

```cpp
myPropertie.setHitDistribution(sipm::SiPMProperties::HitDistribution::kGaussian);
```

## <a name="contrib"></a>Contributing
SimSiPM is being developed in the contest of FCCSW and IDEA Dual-Readout Calorimeter Software. [I am](#contacts) the main responsible for developement and maintainment of this project. If you have a problem, find a BUG or have any suggestion feel free to open a GitHub Issue or to contact me.

## Cite
Even thou SimSiPM has been used in simulations related to published articles, there is not yet an article about SimSiPM only. So when citing SimSiPM please use:
```
@manual{,
title = {{SimSiPM: a library for SiPM simulation}},
author = {Edoardo, Proserpio},
address = {Como, Italy},
year = 2021,
url = {https://github.com/EdoPro98/SimSiPM}
}
```

## <a name="contacts"></a>Contacts
Author: Edoardo Proserpio  
Email: edoardo.proserpio@gmail.com (private)  
Email: eproserpio@studenti.uninsubria.it (instiutional)
