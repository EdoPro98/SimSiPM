import SiPM
import ROOT
from threading import Thread
from queue import Queue
from time import sleep
from dataclasses import dataclass
import pprint

prop = SiPM.SiPMProperties()
prop.setSampling(1)
prop.setDcrOff()
prop.setXtOff()
prop.setRiseTime(1)
avg_npe = 4


@dataclass
class EventData:
    peak: float
    integral: float
    toa: float
    tot: float
    top: float


def UpdateHisto(queue):
    canvas = ROOT.TCanvas()
    p0 = ROOT.TPad("p0", "p0", 0, 0.5, 0.5, 1)
    p1 = ROOT.TPad("p1", "p0", 0.5, 0.5, 1, 1)
    p2 = ROOT.TPad("p2", "p0", 0, 0, 0.5, 0.5)
    p3 = ROOT.TPad("p3", "p0", 0.5, 0, 1, 0.5)
    pads = [p0, p1, p2, p3]
    [p.Draw() for p in pads]
    hist_1 = ROOT.TH1I("h1", "Peak", 2048, -1, 5 * avg_npe)
    hist_2 = ROOT.TH1I("h2", "Integral", 2048, -1, 1024)
    hist_3 = ROOT.TH1I("h3", "Tot", 512, -1, 512)
    hist_4 = ROOT.TH1I("h4", "Toa", 1024, -1, 7)
    hists = [hist_1, hist_2, hist_3, hist_4]
    for p, h in zip(pads, hists):
        p.cd()
        h.Draw()
    while True:
        while not queue.empty():
            event = queue.get()
            hist_1.Fill(event.peak)
            hist_2.Fill(event.integral)
            hist_3.Fill(event.tot)
            hist_4.Fill(event.toa)
        [p.Modified() for p in pads]
        canvas.Update()
        sleep(25e-3)


def GenerateEvents(queue):
    rng = SiPM.SiPMRandom()
    sensor = SiPM.SiPMSensor(prop)

    while True:
        sensor.resetState()
        nphe = rng.randPoisson(avg_npe)
        if nphe > 0:
            times = rng.randGaussian(25, 0.01, nphe)
            sensor.addPhotons(times)
        sensor.runEvent()
        peak = sensor.signal().peak(20, 250, 0.5)
        integral = sensor.signal().integral(20, 300, 0.5)
        tot = sensor.signal().tot(20, 300, 0.5)
        toa = sensor.signal().toa(20, 300, 0.5)
        top = sensor.signal().top(20, 300, 0.5)
        event = EventData(peak, integral, toa, tot, top)
        queue.put(event)
        sleep(5e-5)


q = Queue(2**20)
t1 = Thread(target=GenerateEvents, args=(q,))
t2 = Thread(target=UpdateHisto, args=(q,))
t1.start()
t2.start()
