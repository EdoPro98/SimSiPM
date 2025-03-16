import SiPM
import numpy as np
import matplotlib.pyplot as plt

rng = SiPM.SiPMRandom()

sensor = SiPM.SiPMSensor()

sensor.setProperty("SignalLength",1000)
sensor.setProperty("FallTimeFast",20)
sensor.setProperty("Dcr",5e6)
sensor.setProperty("Xt",0.5)
sensor.setProperty("DXt",0.5)
sensor.setProperty("Ap",0.5)
sensor.setProperty("Snr",40)
sensor.setProperty("Sampling",0.1)
while True:
    sensor.resetState()
    sensor.runEvent()

    signal = np.array(sensor.signal().waveform())
    hits = sensor.hits()
    hitsGraph = sensor.hitsGraph()
    x = np.arange(0,sensor.properties().signalLength(),sensor.properties().sampling())

    fig,ax = plt.subplots()
    maximum = signal.max()


    for i,h in enumerate(hits):
        if h.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            plt.vlines(h.time(),0, maximum, 'r')
            plt.plot(h.time(),0,'^r',ms=8)
        if (h.hitType() == SiPM.SiPMHit.HitType.kDelayedOpticalCrosstalk) | (h.hitType() == SiPM.SiPMHit.HitType.kOpticalCrosstalk):
            plt.vlines(h.time(),0, maximum, 'g')
            plt.plot(h.time(),0,'^g',ms=8)
        if (h.hitType() == SiPM.SiPMHit.HitType.kFastAfterPulse) | (h.hitType() == SiPM.SiPMHit.HitType.kSlowAfterPulse):
            plt.vlines(h.time(),0, maximum, 'b')
            plt.plot(h.time(),0,'^b',ms=8)
    ax.plot(x,signal,"k",lw=2)
    plt.legend(["Signal","Dcr","Xt","Ap"])
    plt.show()
