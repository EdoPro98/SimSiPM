#!/usr/bin/env python
# -*- coding: utf-8 -*-
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import SiPM
import numpy as np

PAUSE = False

fig,ax = plt.subplots()
line = ax.plot([],[], 'k', lw=1)[0]

prop = SiPM.SiPMProperties()
prop.setSampling(1)
prop.setDcr(500e3)
prop.setAp(0.3)
prop.setSnr(25)
sens = SiPM.SiPMSensor(prop)
rng = SiPM.SiPMRandom()
ax.set(xlim=[0,prop.signalLength()])

def on_click(event):
    global PAUSE
    PAUSE ^= True

fig.canvas.mpl_connect('button_press_event', on_click)

MAX_AVG = 0
N = 0
def update(frame):
    global PAUSE
    global MAX_AVG
    global N
    if not PAUSE:
        x = np.arange(0,prop.signalLength(),prop.sampling())
        sens.resetState()
        n = rng.randPoisson(5)
        N+=1
        if n:
            t = rng.randGaussian(10,0.1,n)
            sens.addPhotons(t)
            sens.runEvent()
            y = np.array(sens.signal().waveform())
            if y.max() == np.nan:
                plt.pause(0)
            if y.max() == np.inf:
                for i in y:
                    print(i)
                plt.pause(0)
            MAX_AVG += y.max()
            avg = MAX_AVG/N
            line.set_xdata(x)
            line.set_ydata(y)
            ax.set_ylim(-0.1, avg+3*avg**0.5)
        return (line,)

anim = animation.FuncAnimation(fig=fig, func=update, frames=None, save_count=0, interval=1)
plt.show()
