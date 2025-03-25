import SiPM
import numpy as np
import ROOT

ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(10)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetTextFont(42)
ROOT.gROOT.SetBatch(True)

ROOT.gStyle.SetImageScaling(3.0)

rng = SiPM.SiPMRandom()

prop = SiPM.SiPMProperties()
prop.setDcr(1e6)
prop.setAp(0.10)
prop.setXt(0.03)
prop.setDXt(0.5)
prop.setDXtTau(1)
prop.setTauApFastComponent(10)
prop.setTauApSlowComponent(10)

# prop.setXtOff()
# prop.setApOff() 

prop.setSignalLength(10_000)
sensor = SiPM.SiPMSensor(prop)

mg = ROOT.TMultiGraph()
xbins = np.geomspace(0.1, prop.signalLength(), 257)
hist = ROOT.TH1D('h1','Delays',xbins.size-1,xbins)

gdcr = ROOT.TGraph()
gdcr.SetMarkerColor(ROOT.kRed+1)
gdcr.SetMarkerSize(0.5)
gdcr.SetMarkerStyle(ROOT.kFullCircle)

gap = ROOT.TGraph()
gap.SetMarkerColor(ROOT.kGreen+1)
gap.SetMarkerSize(0.5)
gap.SetMarkerStyle(ROOT.kFullCircle)

gxt = ROOT.TGraph()
gxt.SetMarkerColor(ROOT.kBlue+1)
gxt.SetMarkerSize(0.5)
gxt.SetMarkerStyle(ROOT.kFullCircle)

gdxt = ROOT.TGraph()
gdxt.SetMarkerColor(ROOT.kMagenta+1)
gdxt.SetMarkerSize(0.5)
gdxt.SetMarkerStyle(ROOT.kFullCircle)

gall = ROOT.TGraph()
gall.SetMarkerColor(ROOT.kBlack)
gall.SetMarkerSize(0.5)
gall.SetMarkerStyle(ROOT.kFullCircle)

mg.Add(gdcr,"p")
mg.Add(gap,"p")
mg.Add(gxt,"p")
mg.Add(gdxt,"p")

N = 100_000
for evt in range(N):
    sensor.resetState()
    sensor.runEvent()

    hits = np.array(sensor.hits())

    times = np.array([h.time() for h in hits])
    sortIdx = np.argsort(times)
    hits = hits[sortIdx]

    dcr = []
    ap = []
    xt = []
    dxt = []
    for h in hits:
        if h.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            dcr.append(h)
        if h.hitType() == SiPM.SiPMHit.HitType.kFastAfterPulse:
            ap.append(h)
        if h.hitType() == SiPM.SiPMHit.HitType.kSlowAfterPulse:
            ap.append(h)
        if h.hitType() == SiPM.SiPMHit.HitType.kOpticalCrosstalk:
            xt.append(h)
        if h.hitType() == SiPM.SiPMHit.HitType.kDelayedOpticalCrosstalk:
            dxt.append(h)

    for i,h in enumerate(dcr):
        if i:
            delay = h.time() - dcr[i-1].time()
        else:
            delay = h.time()
        amplitude = h.amplitude()
        gall.AddPoint(delay, amplitude)
        gdcr.AddPoint(delay, amplitude)
        hist.Fill(delay)

    for i,h in enumerate(ap):
        parent = h.parent()
        amplitude = h.amplitude()
        while parent.parent() is not None:
            amplitude += parent.amplitude()
            parent = parent.parent()
        delay = h.time() - parent.time()
        gall.AddPoint(delay, amplitude)
        gap.AddPoint(delay, amplitude)
        hist.Fill(delay)

    for i,h in enumerate(xt):
        parent = h.parent()
        amplitude = h.amplitude()
        while True:
            if parent is None:
                break
            amplitude += parent.amplitude()
            parent = parent.parent()
        if i:
            delay = h.time() - xt[i-1].time()
        else:
            delay = h.time()
        gall.AddPoint(delay, amplitude)
        gxt.AddPoint(delay, amplitude)

    for i,h in enumerate(dxt):
        parent = h.parent()
        amplitude = h.amplitude()
        while parent.parent() is not None:
            amplitude += parent.amplitude()
            parent = parent.parent()
        delay = h.time() - parent.time()
        gall.AddPoint(delay, amplitude)
        gdxt.AddPoint(delay, amplitude)

c1 = ROOT.TCanvas()
c2 = ROOT.TCanvas()
c3 = ROOT.TCanvas()
c4 = ROOT.TCanvas()
c1.SetLogx()
c2.SetLogx()
c4.SetLogx()

c1.cd()
legend_color = ROOT.TLegend(0.15,0.75,0.25,0.85)
legend_color.AddEntry(gdcr,'DCR','p')
legend_color.AddEntry(gap,'AP','p')
legend_color.AddEntry(gxt,'XT','p')
legend_color.AddEntry(gdxt,'DeXT','p')
mg.Draw("ap")
legend_color.Draw()
mg.SetTitle("")
mg.GetXaxis().SetTitle('Delay [ns]')
mg.GetYaxis().SetTitle('Amplitude [A.U.]')

c2.cd()
gall.Draw("ap")
gall.SetTitle("")
gall.GetXaxis().SetTitle('Delay [ns]')
gall.GetYaxis().SetTitle('Amplitude [A.U.]')

c3.cd()
c3.SetLogx()
c3.SetLogy()
fitFun = ROOT.TF1('f1','[N]*x*exp(-[DCR]*x)',0,prop.signalLength())
fitFun.SetParameter('DCR',prop.dcr()/1e9)
hist.SetTitle("")
hist.Fit(fitFun,'E0','', 200.0, prop.signalLength()-500.0)
rp = ROOT.TRatioPlot(hist,'diff')
rp.SetFitDrawOpt('0')
rp.SetGraphDrawOpt('p')
rp.SetH1DrawOpt('e')
rp.Draw('noconfint')
rp.GetUpperPad().cd()
rp.GetUpperPad().SetLogy()
rp.GetLowerRefYaxis().SetRangeUser(-1,15)
fitFun.DrawF1(xbins[0],prop.signalLength(),'same')

c4.cd()
hist_c = hist.GetCumulative()
for i in range(hist_c.GetNbinsX()+1):
    hist_c.SetBinContent(i,hist.GetEntries() - hist_c.GetBinContent(i))
hist_c.Scale(1/hist.GetEntries())
fitFun = ROOT.TF1('f2','(1-[AP])*exp(-[DCR]*x)',0,prop.signalLength())
fitFun.SetParameter('DCR',1e-9*prop.dcr())
fitFun.SetParameter('AP',prop.ap())
hist_c.Fit(fitFun,'E0','',100,5000)
hist_c.Draw()
hist_c.GetXaxis().SetTitle('Delay [ns]')
hist_c.GetYaxis().SetTitle('CCDF')
fitFun.DrawF1(xbins[0],xbins[-1],'L SAME')
fitFun.SetLineColor(ROOT.kRed)
fitFun.SetLineWidth(3)

c1.Update()
c2.Update()
c3.Update()
c4.Update()

c1.SaveAs('DelaysColored.pdf')
c2.SaveAs('DelaysBlack.pdf')
c3.SaveAs('DelaysHist.pdf')
c4.SaveAs('DelaysCHist.pdf')
input()
