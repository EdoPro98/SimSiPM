import SiPM
import matplotlib.pyplot as plt
import numpy as np

s = SiPM.SiPMSensor()
s.resetState()
s.addPhoton(-100)
s.runEvent()
t = np.arange(0, s.properties().signalLength(), s.properties().sampling())
plt.plot(t, s.signal().waveform())
plt.show()
