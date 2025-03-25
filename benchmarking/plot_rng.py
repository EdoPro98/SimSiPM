import numpy as np
import json
import ROOT

ROOT.gStyle.SetTitleFont(42)
ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptFit(11)


f_rng = open("rng.json", "r")

jf_rng = json.load(f_rng)

bytes = []
rate_rng = []
rate_e_rng = []

for k in jf_rng["benchmarks"]:
    if "MultipleRng" in k["name"] and "mean" in k["name"]:
        bytes.append(int(k["Bytes"]))
        rate_rng.append(int(k["Rate"]))
    if "MultipleRng" in k["name"] and "std" in k["name"]:
        rate_e_rng.append(int(k["Rate"]))

bytes = np.array(bytes) / 1024**3
rate_rng = np.array(rate_rng) / 1024**3
rate_e_rng = np.array(rate_e_rng) / 1024**3

c1 = ROOT.TCanvas()
gr1 = ROOT.TGraph(bytes.size, bytes, rate_rng)
gr1.SetTitle("")
gr1.SetMarkerStyle(ROOT.kFullCircle)
gr1.Draw("AP")
gr1.GetXaxis().SetTitle("Array size [GB]")
gr1.GetYaxis().SetTitle("Speed [GB/s]")
c1.Update()
c1.SaveAs("genspeedfull.pdf")

c2 = ROOT.TCanvas("", "")
idx = np.argwhere(bytes > 7.5)[0][0]
gr2 = ROOT.TGraphErrors(
    bytes[idx:].size, bytes[idx:], rate_rng[idx:], 0, rate_e_rng[idx:]
)
gr2.SetMarkerStyle(ROOT.kFullCircle)
fit = gr2.Fit("pol0")
gr2.GetFunction("pol0").SetLineColor(ROOT.kRed)

gr2.SetTitle("")
gr2.Draw("AP")
gr2.GetXaxis().SetTitle("Array size [GB]")
gr2.GetYaxis().SetTitle("Speed [GB/s]")
c2.Update()
c2.SaveAs("genspeed.pdf")
