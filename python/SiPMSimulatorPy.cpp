#include "SiPMSensor.h"
#include "SiPMSimulator.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace sipm;
using std::vector;

void SiPMSimulatorPy(py::module &m) {
  py::class_<SiPMSimulator> SiPMSimulator(m, "SiPMSimulator");
  SiPMSimulator
    .def(py::init<SiPMSensor *>())
    //.def("readConfiguration",&SiPMSimulator::readConfiguration)
    .def("addEvents", py::overload_cast<const vector<vector<double>> &>(&SiPMSimulator::addEvents))
    .def("addEvents",
         py::overload_cast<const vector<vector<double>> &, const vector<vector<double>> &>(&SiPMSimulator::addEvents))
    .def("push_back", py::overload_cast<const vector<double> &>(&SiPMSimulator::push_back))
    .def("push_back", py::overload_cast<const vector<double> &, const vector<double> &>(&SiPMSimulator::push_back))
    .def("setSensor", &SiPMSimulator::setSensor)
    .def("setIntegration", &SiPMSimulator::setIntegration)
    .def("clear", &SiPMSimulator::clear)
    .def("sensor", static_cast<const SiPMSensor *(SiPMSimulator::*)() const>(&SiPMSimulator::sensor))
    .def("sensor", static_cast<SiPMSensor *(SiPMSimulator::*)()>(&SiPMSimulator::sensor))
    .def("runSimulation", &SiPMSimulator::runSimulation)
    .def("getResults", &SiPMSimulator::getResults)
    .def("getResult", &SiPMSimulator::getResult);

  py::class_<SiPMSimulator::SiPMResult> SiPMResult(SiPMSimulator, "SiPMResult");
  SiPMResult.def(py::init<>())
    .def_readonly("ID", &SiPMSimulator::SiPMResult::idx)
    .def_readonly("Integral", &SiPMSimulator::SiPMResult::integral)
    .def_readonly("Peak", &SiPMSimulator::SiPMResult::peak)
    .def_readonly("Toa", &SiPMSimulator::SiPMResult::toa)
    .def_readonly("Tot", &SiPMSimulator::SiPMResult::tot)
    .def_readonly("Top", &SiPMSimulator::SiPMResult::top);
}
