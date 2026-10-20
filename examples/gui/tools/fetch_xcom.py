#!/usr/bin/env python3
"""Downloads elemental photon cross sections from NIST XCOM.

https://physics.nist.gov/PhysRefData/Xcom/html/xcom1.html

For each element used by the scintillators in data/scintillators.json it
writes data/xcom/<Symbol>.csv with the XCOM standard energy grid:
  energy [MeV], incoherent, photoelectric, pair (nuclear), pair (electron) [cm2/g]
At absorption edges XCOM lists the edge energy twice (below / above); both
rows are kept in order. The K-edge energy is written in the header.

Usage: python3 fetch_xcom.py
"""

import html
import os
import re
import urllib.parse
import urllib.request

ELEMENTS = {"H": 1, "C": 6, "O": 8, "Na": 11, "Al": 13, "Si": 14, "Ga": 31, "Ge": 32, "Br": 35,
            "Y": 39, "I": 53, "Cs": 55, "La": 57, "Ce": 58, "Gd": 64, "Lu": 71, "W": 74,
            "Pb": 82, "Bi": 83}
URL = "https://physics.nist.gov/cgi-bin/Xcom/xcom3_1"


def fetch(z):
    data = urllib.parse.urlencode({
        "ZNum": z, "ZSym": "", "OutOpt": "PIC", "Graph0": "on", "NumAdd": "1", "Energies": "",
        "Output": "on", "WindowXmin": "0.001", "WindowXmax": "100", "ResizeFlag": "on"}).encode()
    req = urllib.request.Request(URL, data=data, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=120) as r:
        text = r.read().decode("utf-8", "replace")
    text = html.unescape(re.sub(r"<[^>]*>", " ", text))
    rows, k_edge = [], None
    for line in text.splitlines():
        tok = line.split()
        # Edge rows start with "<Z> K 1"; plain rows have 8 numeric columns
        label = None
        if len(tok) in (10, 11) and tok[0] == str(z):
            label, tok = tok[1], tok[-8:]  # "<Z> K" or "<Z> L 3"
        if len(tok) != 8:
            continue
        try:
            e, coh, inc, pe, pn, pel, tot, totnc = map(float, tok)
        except ValueError:
            continue
        if label == "K":
            k_edge = e
        rows.append((e, inc, pe, pn, pel))
    return rows, k_edge


def main():
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "xcom")
    os.makedirs(out_dir, exist_ok=True)
    for sym, z in ELEMENTS.items():
        rows, k_edge = fetch(z)
        with open(os.path.join(out_dir, f"{sym}.csv"), "w") as f:
            f.write(f"# {sym} (Z={z}) photon cross sections from NIST XCOM ({URL})\n")
            f.write(f"# K-edge MeV: {k_edge if k_edge else 0}\n")
            f.write("# energy_MeV,incoherent,photoelectric,pair_nuclear,pair_electron (cm2/g)\n")
            for r in rows:
                f.write(",".join(f"{v:.4e}" for v in r) + "\n")
        print(f"{sym}: {len(rows)} rows, K edge {k_edge}")


if __name__ == "__main__":
    main()
