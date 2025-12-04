#include <TPad.h>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include <TApplication.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TGraphErrors.h>
#include <TMath.h>
#include <TMultiGraph.h>
#include <TROOT.h>
#include <TStyle.h>
#include <sipm/SiPMProperties.h>
#include <sipm/SiPMSensor.h>

static constexpr int N_TEST = 32;
static constexpr int N_AVG = 16;
static constexpr int N_PHE_MAX = 1000;
static constexpr int N_REPEAT = 100000;

int main(int argc, char* argv[]) {
  gROOT->SetStyle("ATLAS");
  TApplication* app = new TApplication("SiPM Benchmark", &argc, argv);

  sipm::SiPMProperties properties;
  sipm::SiPMSensor sensor(properties);

  // Store averaged results
  std::array<double, N_TEST> avg_duration;
  std::array<double, N_TEST> std_duration;

  TCanvas canvas("c1", "SiPM Benchmark", 1920, 1080);
  canvas.Divide(1, 2, 0, 0);
  canvas.cd(1);
  gStyle->SetPadBorderMode(0);
  gStyle->SetFrameBorderMode(0);
  TGraphErrors data_graph;
  data_graph.SetTitle("a");
  data_graph.GetXaxis()->SetTitle("#phe");
  data_graph.GetYaxis()->SetTitle("Time [#mus]");
  data_graph.Draw("apc plc pmc");
  gPad->SetBottomMargin(0);

  canvas.cd(2);
  gPad->SetGrid();
  TGraphErrors data_graph2;
  data_graph2.SetTitle("b");
  data_graph2.GetXaxis()->SetTitle("#phe");
  data_graph2.GetYaxis()->SetTitle("Speed [phe/#mus]");
  data_graph2.Draw("apc plc pmc");
  gPad->SetTopMargin(0);
  gPad->SetTickx();

  canvas.Modified();
  canvas.Update();

  // Loop over different photon number
  for (size_t i = 0; i < N_TEST; i++) {
    const uint32_t avgNpe = i == 0 ? 1 : i * (N_PHE_MAX / (N_TEST - 1));

    std::cout << ">>> Starting benchmark with " << avgNpe << " photons <<<" << std::endl;

    std::vector<double> all_durations;
    // Loop for averaging results
#pragma omp parallel for firstprivate(sensor)
    for (int test_i = 0; test_i < N_AVG; test_i++) {
      std::array<double, N_REPEAT> durations;
      auto rng = sipm::SiPMRandom();
      for (int j = 0; j < N_REPEAT; j++) {
        const int npe = rng.randPoisson(avgNpe);
        sensor.resetState();
        if (npe > 0) {
          const std::vector<double> times = rng.randGaussian(20, 0.1, npe);
          sensor.addPhotons(times);
        }
        asm("mfence" ::: "memory");
        const auto begin = std::chrono::high_resolution_clock::now();
        sensor.runEvent();
        const auto end = std::chrono::high_resolution_clock::now();
        asm("mfence" ::: "memory");
        durations[j] = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
      }

#pragma omp critical
      {
        std::copy(durations.begin(), durations.end(), std::back_inserter(all_durations));
        // Updating plot
        std::vector<double> tmp;
        for (auto& x : all_durations) {
          if (x > 0) {
            tmp.push_back(x);
          }
        }
        const double mean = TMath::Mean(tmp.begin(), tmp.end());
        const double std = TMath::StdDev(tmp.begin(), tmp.end());
        data_graph.SetPoint(i, avgNpe, mean);
        data_graph.SetPointError(i, 0, std);
        data_graph.GetXaxis()->SetLimits(0, avgNpe * 1.1);
        canvas.cd(1);
        gPad->Modified();

        const double effective_duration = i == 0 ? mean : mean - avg_duration[0];
        const double std_effective_duration = i == 0 ? std : sqrt(std * std + std_duration[0] * std_duration[0]);
        const double speed = avgNpe / effective_duration;
        const double e_speed = avgNpe * std_effective_duration / (effective_duration * effective_duration);
        data_graph2.SetPoint(i, avgNpe, speed);
        data_graph2.SetPointError(i, 0, e_speed);
        data_graph2.GetXaxis()->SetLimits(0, avgNpe * 1.1);
        canvas.cd(2);
        gPad->Modified();

        canvas.UpdateAsync();
      }
    }

    avg_duration[i] = TMath::Mean(all_durations.begin(), all_durations.end());
    std_duration[i] = TMath::StdDev(all_durations.begin(), all_durations.end());

    data_graph.SetPoint(i, avgNpe, avg_duration[i]);
    data_graph.SetPointError(i, 0, std_duration[i]);
    canvas.cd(1);
    data_graph.GetXaxis()->SetLimits(0, avgNpe * 1.1);
    gPad->Modified();

    const double effective_duration = i == 0 ? avg_duration[i] : avg_duration[i] - avg_duration[0];
    const double std_effective_duration =
      i == 0 ? std_duration[i] : sqrt(std_duration[i] * std_duration[i] + std_duration[0] * std_duration[0]);
    const double speed = avgNpe / effective_duration;
    const double e_speed = avgNpe * std_effective_duration / (effective_duration * effective_duration);

    data_graph2.SetPoint(i, avgNpe, speed);
    data_graph2.SetPointError(i, 0, e_speed);
    data_graph2.GetXaxis()->SetLimits(0, avgNpe * 1.1);
    canvas.cd(2);
    gPad->Modified();

    canvas.UpdateAsync();

    std::cout << "Performance: " << data_graph.GetPointY(i) << " +/- " << data_graph.GetErrorY(i) << " us\n";
    std::cout << "Performance: " << data_graph2.GetPointY(i) << " +/- " << data_graph2.GetErrorY(i) << " phe/us\n";
    std::cout << std::endl;
  }
  app->Run();
  return 0;
}
