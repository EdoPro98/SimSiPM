# SimSiPM
![GitHub release](https://img.shields.io/github/v/release/EdoPro98/SimSiPM?include_prereleases)

![GCC](https://github.com/EdoPro98/SimSiPM/workflows/GCC/badge.svg?branch=master&event=push)
![AppleClang](https://github.com/EdoPro98/SimSiPM/workflows/AppleClang/badge.svg?branch=master&event=push)

![GitHub issues](https://img.shields.io/github/issues/EdoPro98/SimSiPM)
![GitHub last commit](https://img.shields.io/github/last-commit/EdoPro98/SimSiPM)
[![GitHub license](https://img.shields.io/github/license/EdoPro98/SimSiPM)](https://github.com/EdoPro98/SimSiPM/blob/master/LICENSE)

![PyPI - Downloads](https://img.shields.io/pypi/dd/SiPM)

## Table of contents
1. [Introduction](#introduction)  
2. [Features](#features)  
3. [Installation](#installation)
  1. [C++](#c++install)
  2. [Python](#pyinstall)
4. [C++ Basic use](#c++_basic_bsage)  
5. [Python Basic use](#python_basic_usage)  
6. [Advanced use](#advanced_use)  
  1. [Pde](#pde)
  2. [Hit distribution](#hit)

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
make -C install
```
Installation directory can be specified with `-DCMAKE_INSTALL_PREFIX` variable.  
Python bindings can be installed in the default python site-packages path by adding the variable `-DCOMPILE_PYTHON_BINDINGS=ON` but this requires Pybind11 to be installed.

### <a name="pyinstall"></a>Python
It is also possible to install only the python version via pip but performance might not be as good as the source code version:
```sh
pip install SiPM
```

## <a name="C++_basic_bsage"></a>C++ basic use

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
mySensor.addPhoton(times);    // Sets photon times (times are in ns) (not appending)
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

## Contibute
SimSiPM is being developed in the contest of FCCSW and IDEA Dual-Readout Calorimeter Software. [I](#contacts) am the main responsible for developement and maintainment of this project, and if you have an

## <a name="contacts"></a>Contacts
Author: Edoardo Proserpio  
Email: edoardo.proserpio@gmail.com (private)  
Email: eproserpio@studenti.uninsubria.it (instiutional)
