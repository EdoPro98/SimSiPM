import SiPM
import matplotlib.pyplot as plt
import numpy as np

rng = SiPM.SiPMRandom()

sensor1 = SiPM.SiPMSensor()
sensor2 = SiPM.SiPMSensor()
sensor3 = SiPM.SiPMSensor()

sensor1.setProperty("SignalLength",1000)
sensor2.setProperty("SignalLength",1000)
sensor3.setProperty("SignalLength",1000)

sensor1.setProperty("Sampling",0.1)
sensor2.setProperty("Sampling",0.1)
sensor3.setProperty("Sampling",0.1)

sensor1.setProperty("FallTimeFast",50)

sensor2.setProperty("RiseTime",10)
sensor2.setProperty("FallTimeFast",80)

sensor3.setProperty("SlowComponentFraction",0.10)
sensor3.setProperty("FallTimeFast",5)
sensor3.setProperty("FallTimeSlow",80)
sensor3.properties().setSlowComponentOn()

sensor1.setProperty("Snr",30)
sensor2.setProperty("Snr",30)
sensor3.setProperty("Snr",30)

t = [250]

sensor1.addPhotons(t)
sensor2.addPhotons(t)
sensor3.addPhotons(t)

sensor1.runEvent()
sensor2.runEvent()
sensor3.runEvent()

signal1 = sensor1.signal().waveform()
signal2 = sensor2.signal().waveform()
signal3 = sensor3.signal().waveform()


sensor1.resetState()
sensor2.resetState()
sensor3.resetState()
t = np.array(rng.Rand(20))*1000
sensor1.addPhotons(t)
sensor2.addPhotons(t)
sensor3.addPhotons(t)

sensor1.runEvent()
sensor2.runEvent()
sensor3.runEvent()

signal4 = sensor1.signal().waveform()
signal5 = sensor2.signal().waveform()
signal6 = sensor3.signal().waveform()


time = np.arange(0,sensor1.properties().signalLength(),sensor1.properties().sampling())

fig1,ax1 = plt.subplots(3,1)
for a in ax1:
    a.grid()

ax1[0].plot(time,signal1,"r",lw=1)
ax1[1].plot(time,signal2,"g",lw=1)
ax1[2].plot(time,signal3,"b",lw=1)

ax1[0].set_title("Rising time: 1 ns - Falling time: 20 ns")
ax1[1].set_title("Rising time: 5 ns - Falling time: 80 ns")
ax1[2].set_title("Rising time: 1 ns - Falling time fast: 5 ns - Falling time slow: 80 ns")
plt.tight_layout()

fig2,ax2 = plt.subplots(3,1)

for a in ax2:
    a.grid()

ax2[0].plot(time,signal4,"r",lw=1)
ax2[1].plot(time,signal5,"g",lw=1)
ax2[2].plot(time,signal6,"b",lw=1)

ax2[0].set_title("Rising time: 1 ns - Falling time: 20 ns")
ax2[1].set_title("Rising time: 5 ns - Falling time: 80 ns")
ax2[2].set_title("Rising time: 1 ns - Falling time fast: 5 ns - Falling time slow: 80 ns")
plt.tight_layout()

plt.show()
