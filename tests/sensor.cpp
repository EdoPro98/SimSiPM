#include <gtest/gtest.h>
#include <stdint.h>
#include "SiPM.h"

#include <iostream>

using namespace sipm;

struct TestSiPMSensor : public ::testing::Test{
  SiPMSensor sut;
  SiPMRandom rng;
};

TEST_F(TestSiPMSensor, Constructor){
  SiPMSensor sensor = SiPMSensor();
}

TEST_F(TestSiPMSensor, AddPhoton){
  const int N = 100000;
  for(int i=0; i<1000;++i){
    sut.resetState();
    sut.addPhoton(10);
  }
}

TEST_F(TestSiPMSensor, AddPhotons){
  const int N = 100000;
  for(int i=0; i<N;++i){
    sut.resetState();
    int n = rng.randInteger(100);
    std::vector<double> t = rng.randGaussian(100,0.2,n);
    sut.addPhotons(t);
  }
}

TEST_F(TestSiPMSensor, AddPhotonWlen){
  const int N = 100000;
  for(int i=0; i<1000;++i){
    sut.resetState();
    sut.addPhoton(rng.randGaussian(100,0.2),rng.randGaussian(450,20));
  }
}

TEST_F(TestSiPMSensor, AddPhotonsWlen){
  const int N = 100000;
  sut.resetState();
  for(int i=0; i<N;++i){
    sut.resetState();
    int n = rng.randInteger(100);
    std::vector<double> t = rng.randGaussian(100,0.2, n);
    std::vector<double> w = rng.randGaussian(450,20, n);
    sut.addPhotons(t,w);
  }
}

TEST_F(TestSiPMSensor, AddDcr){
  const int N = 500000;
  int ndcr = 0;
  SiPMSensor sensor;
  for(int i=0; i<N; ++i){
    sensor.resetState();
    sensor.runEvent();
    ndcr += sensor.debug().nDcr;
  }
  double rate = 1e9 * ((double)ndcr / N / sensor.properties().signalLength());
  EXPECT_GE(rate, sensor.properties().dcr()*0.95);
  EXPECT_LE(rate, sensor.properties().dcr()*1.05);
}

TEST_F(TestSiPMSensor, AddXt){
  const int N = 500000;
  int nxt = 0;
  int npe = 0;
  SiPMSensor sensor;
  for(int i=0; i<N; ++i){
    sensor.resetState();
    sensor.runEvent();
    nxt += sensor.debug().nXt;
    npe += sensor.debug().nDcr;
  }
  double xt = (double)nxt / (double)npe;
  EXPECT_GE(xt, sensor.properties().xt()*0.95);
  EXPECT_LE(xt, sensor.properties().xt()*1.05);
}


TEST_F(TestSiPMSensor, AddAp){
  const int N = 500000;
  int nap = 0;
  int npe = 0;
  SiPMSensor sensor;
  for(int i=0; i<N; ++i){
    sensor.resetState();
    sensor.runEvent();
    nap += sensor.debug().nAp;
    npe += sensor.debug().nDcr;
  }
  double ap = (double)nap / (double)npe;
  EXPECT_GE(ap, sensor.properties().ap()*0.95);
  EXPECT_LE(ap, sensor.properties().ap()*1.05);
}

TEST_F(TestSiPMSensor, SignalGeneration){
  const int N = 10;
  const int R = 1000;
  SiPMSensor lsut = sut;
  lsut.properties().setXtOff();
  lsut.properties().setDcrOff();
  lsut.properties().setApOff();

  for(int i=0;i<N;++i){
    std::vector<double> t = rng.randGaussian(100, 0.1, i);
    double avg_max = 0;
    for(int j=0;j<R;++j){
      lsut.addPhotons(t);
      lsut.runEvent();
      SiPMAnalogSignal signal = lsut.signal();
      double max = 0;
      for(int k=0; k<lsut.properties().nSignalPoints(); ++k){
        if(signal[k] > max){
          max = signal[k];
        }
      }
      avg_max += max;
      lsut.resetState();
    }
    avg_max /= R;
    std::cerr<<i<<" "<<avg_max<<"\n";
    EXPECT_GE(avg_max+0.5,i);
    EXPECT_LE(avg_max-0.5,i);
  }
}
