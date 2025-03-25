import SiPM
import ROOT
import numpy as np
from uncertainties import ufloat
from uncertainties.umath import log
from tqdm import trange
ROOT.gStyle.SetLineScalePS(1)
ROOT.gStyle.SetOptFit(1)
ROOT.gStyle.SetOptStat(10)
ROOT.gStyle.SetOptTitle(0)

ROOT.gStyle.SetImageScaling(3.)
ROOT.gROOT.SetBatch(True)

N = 1_000_000

prop = SiPM.SiPMProperties()
prop.setXtOff()
prop.setApOff()
prop.setDcr(1000e3)
prop.setSignalLength(10000)
sensor = SiPM.SiPMSensor(prop)

xbins = np.arange(32)-0.5
hist_time = ROOT.TH1D('h1','DCR time distribution',64,0,prop.signalLength())
hist_delay = ROOT.TH1D('h2','DCR time delay',64,0,1e-9*prop.signalLength())
hist_count = ROOT.TH1D('h3','DCR counts',xbins.size-1,xbins)
hist_peak = ROOT.TH1D('h4','Peak value',512, -0.1, 2)
hist_peak_cut = ROOT.TH1D('h5','Peak value with cuts',512, -0.1, 2)

binw_peak = hist_peak.GetBinCenter(2) - hist_peak.GetBinCenter(1)

INT_TIME = 250
dcrs = []
ndcr = 0
for i in trange(N):
    sensor.resetState()
    sensor.runEvent()
    peak = sensor.signal().peak(500,INT_TIME,0.0)
    toa = sensor.signal().toa(500,INT_TIME,0.1)

    hist_peak.Fill(peak)

    hits = sensor.hits()

    ndcr += len(hits)
    hist_count.Fill(len(hits))

    mask = True
    for j,h in enumerate(hits):
        if not h.hitType() == SiPM.SiPMHit.HitType.kDarkCount:
            continue
        if 250 < h.time() < 500:
            mask = False
        hist_time.Fill(h.time())
        dcrs.append(h.time() + i*prop.signalLength())
    if mask:
        hist_peak_cut.Fill(peak)

delays = np.diff(dcrs, prepend=0)*1e-9
[hist_delay.Fill(d) for d in delays]

hist_time.Scale(hist_time.GetNbinsX()/(N*prop.signalLength()*1e-9))

c_time = ROOT.TCanvas()
c_delay = ROOT.TCanvas()
c_count = ROOT.TCanvas()
c_peak = ROOT.TCanvas()

c_time.cd()
fit = hist_time.Fit('pol0','E0','',10,prop.signalLength()-10)
hist_time.GetFunction('pol0').SetLineColor(2)
tp_time = ROOT.TRatioPlot(hist_time,'diffsig')
tp_time.SetGraphDrawOpt('p')
tp_time.Draw('hideup noconfint')
tp_time.GetUpperRefYaxis().SetRangeUser(0.95*prop.dcr(),1.05*prop.dcr())
tp_time.GetUpperRefYaxis().SetTitle("Rate [Hz]")
tp_time.GetLowerRefYaxis().SetTitle("#frac{Residuals}{#sigma}")
tp_time.GetUpperRefXaxis().SetTitle("Time [ns]")
tp_time.GetLowerRefGraph().SetMinimum(-3)
tp_time.GetLowerRefGraph().SetMaximum(3)
tp_time.GetLowYaxis().SetNdivisions(7,0,0)

c_delay.cd()
c_delay.SetLogy()
fitFun = ROOT.TF1('f1','exp(-[DCR]*x+[Constant])',0,1e-9*prop.signalLength())
fitFun.SetParameter('DCR',1/hist_delay.GetMean())
fitFun.SetParameter('Constant',10)
fit = hist_delay.Fit(fitFun,'ELM','e',0,1e-9*prop.signalLength())
hist_delay.GetFunction('f1').SetLineColor(2)
hist_delay.GetXaxis().SetTitle('Delay [ns]')
hist_delay.GetYaxis().SetTitle('Counts')

c_count.cd()
poiss_hist = ROOT.TH1D()
hist_count.Copy(poiss_hist)
hist_count.SetLineColor(ROOT.kBlue)
for i in range(poiss_hist.GetNbinsX()):
    x = poiss_hist.GetBinCenter(i)
    y = N*ROOT.TMath.Poisson(x,prop.dcr()*prop.signalLength()*1e-9)
    poiss_hist.SetBinContent(i,y)
poissFun = ROOT.TF1('f2','[N]*TMath::Poisson(x,[Mean])',0,30)
poissFun.SetParameter('Mean',hist_count.GetMean())
poissFun.SetParameter('N',N)
fit = hist_count.Fit(poissFun,'EL','',0,30)
fitLine = hist_count.GetFunction('f2')
fitLine.SetLineColor(2)
hist_count.SetMarkerStyle(ROOT.kFullCircle)
tp_count = ROOT.TRatioPlot(poiss_hist,hist_count, 'diffsig')
tp_count.SetGraphDrawOpt('p')
tp_count.Draw('hideup')
tp_count.GetLowerRefGraph().SetMaximum(3)
tp_count.GetLowerRefGraph().SetMinimum(-3)
tp_count.GetLowYaxis().SetNdivisions(7,0,0)
tp_count.GetUpperRefXaxis().SetTitle('DCR events')
tp_count.GetUpperRefYaxis().SetTitle('Counts')
legend = ROOT.TLegend(0.6,0.2,0.85,0.4)
legend.AddEntry(fitLine,'Fit','l')
legend.AddEntry(poiss_hist,'Expected Poissonian','l')
legend.AddEntry(hist_count,'Data','lep')
tp_count.GetUpperPad().cd()
legend.Draw()

c_peak.cd()
c_peak.SetLogy()
fitFun1 = ROOT.TF1('f4','gaus',-1,3)
fitFun2 = ROOT.TF1('f5','gaus',-1,3)
fitFun1.SetParameter(0,hist_peak.GetMaximum())
fitFun1.SetParameter('Mean',0.05)
fitFun1.SetParameter('Sigma',0.01)
fitFun2.SetParameter(0,hist_peak.GetMaximum())
fitFun2.SetParameter('Mean',1)
fitFun2.SetParameter('Sigma',0.05)
hist_peak.Fit(fitFun1,'L0','',-0.1,0.2)
hist_peak.Fit(fitFun2,'L0','',0.8,1.2)
fitRes1 = hist_peak.Fit(fitFun1,'LES+','E',fitFun1.GetParameter(1)-3*fitFun1.GetParameter(2),fitFun1.GetParameter(1)+3*fitFun1.GetParameter(2))
fitRes2 = hist_peak.Fit(fitFun2,'LES+','E',fitFun2.GetParameter(1)-2*fitFun2.GetParameter(2),fitFun2.GetParameter(1)+2*fitFun2.GetParameter(2))
hist_peak.GetFunction('f4').SetLineColor(ROOT.kRed)
hist_peak.SetLineWidth(2)
hist_peak_cut.Draw('e same')
hist_peak_cut.SetLineColor(ROOT.kOrange)
ROOT.gPad.cd()
line = ROOT.TLine(0.15,0,0.15,hist_peak.GetMaximum())
line.SetLineColor(ROOT.kBlack)
line.SetLineStyle(ROOT.kDashed)
arrow = ROOT.TArrow(0.15, 0.9*hist_peak.GetMaximum(), 0.10, 0.9*hist_peak.GetMaximum(),0.01,'|>')
arrow.SetFillColor(ROOT.kBlack)
arrow.SetLineWidth(5)
arrow.Draw()
line.Draw()

# Number of events < 0.5
N0_mc = ufloat(hist_count.GetBinContent(1), hist_count.GetBinError(1))
R0_mc = N0_mc / hist_count.GetEntries()
dcr_mc = -log(R0_mc) / (1e-9*prop.signalLength())
print(f"DCR value from MC truth estimated by 0 events: {dcr_mc.n:.2f}+/-{dcr_mc.s:.2f} Hz")

N1_mc = ufloat(ndcr, ndcr**0.5)
dcr_mc_1 = N1_mc / (1e-9*hist_count.GetEntries()*prop.signalLength())
print(f"DCR value from MC truth estimated by 1 events: {dcr_mc_1.n:.2f}+/-{dcr_mc_1.s:.2f} Hz")


N0_count = hist_peak.Integral(0,hist_peak.FindBin(0.15))
N0_count = ufloat(N0_count, N0_count**0.5)
R0_count = N0_count / hist_peak.GetEntries()
dcr_peak_count = -log(R0_count) / (1e-9*INT_TIME)
print(f"DCR value from peak value estimated by 0 events count: {dcr_peak_count.n:.2f}+/-{dcr_peak_count.s:.2f} Hz")

N0_fit = fitFun1.Integral(0,0.2) / binw_peak
N0_fit_error = fitFun1.IntegralError(0,0.2,fitRes1.GetParams(), fitRes1.GetCovarianceMatrix().GetMatrixArray()) / binw_peak
N0_fit = ufloat(N0_fit, N0_fit_error)
R0_fit = N0_fit / hist_peak.GetEntries()
dcr_peak_fit = -log(R0_fit) / (1e-9*INT_TIME)
print(f"DCR value from peak value estimated by 0 events fit: {dcr_peak_fit.n:.2f}+/-{dcr_peak_fit.s:.2f} Hz")

# Number of events > 0.5
N1_count = hist_peak.Integral(hist_peak.FindBin(0.8), hist_peak.FindBin(1.2))
N1_count = ufloat(N1_count, N1_count**0.5)
dcr_peak_1_count = N1_count / (1e-9*INT_TIME*hist_peak.GetEntries())
print(f"DCR value from peak value estimated by 1 events count: {dcr_peak_1_count.n:.2f}+/-{dcr_peak_1_count.s:.2f} Hz")

N1_fit = fitFun2.Integral(0.5,1.5) / binw_peak
N1_fit_error = fitFun2.IntegralError(0.5,1.5,fitRes2.GetParams(), fitRes2.GetCovarianceMatrix().GetMatrixArray()) / binw_peak
N1_fit = ufloat(N1_fit, N1_fit_error)
dcr_peak_1_fit = N1_fit / (1e-9*INT_TIME*hist_peak.GetEntries())
print(f"DCR value from peak value estimated by 1 events fit: {dcr_peak_1_fit.n:.2f}+/-{dcr_peak_1_fit.s:.2f} Hz")

c_time.Modified()
c_time.Update()

c_delay.Modified()
c_delay.Update()

c_count.Modified()
c_count.Update()

c_peak.Modified()
c_peak.Update()

# c_time.SaveAs('dcrtime.pdf')
# c_delay.SaveAs('dcrdelay.pdf')
# c_count.SaveAs('dcrcount.pdf')
# c_peak.SaveAs('dcrpeak.pdf')
input()
