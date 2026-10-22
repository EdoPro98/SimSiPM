#!/usr/bin/env python3
"""Regenerates the numbers and the figures of README.md on the machine where it runs.

    python docs/update_readme.py                  # everything, 25 repetitions of every benchmark
    python docs/update_readme.py --quick          # a fast smoke test (3 short repetitions)
    python docs/update_readme.py --skip-build --skip-bench   # only figures and README from docs/data

What it does:
  1. builds the library, bench/events and the Python module twice in --build-dir: with the default
     flags and with the native ones (-march=native; the native columns are left out if that build fails)
  2. runs bench/events for both builds, and saves the results and a description of the machine in
     docs/data/bench_default.json and docs/data/bench_native.json
  3. regenerates all the figures of images/ (docs/make_figures.py)
  4. rewrites the Performance section of README.md from docs/performance.md.in and the data
     (the sentences that interpret the numbers are chosen from the numbers), and the
     "typical event" bullet of the features
  5. runs docs/check_readme.py: compiles and runs every code block of the README

The benchmarks are timing measurements: do not run anything else while they run.
Needs: cmake, a C++17 compiler, Google Benchmark, pybind11 and the Python packages of docs/requirements.txt.
"""
import argparse
import datetime
import json
import math
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
DATA = DOCS / "data"
SCAN = [1 << i for i in range(15)]  # photons of the scan benchmark


def say(msg):
    print(f"\n==> {msg}", flush=True)


def run(cmd, **kw):
    print("$", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], check=True, **kw)


# ---------------------------------------------------------------------------------------------
# machine description
# ---------------------------------------------------------------------------------------------
def cpu_name():
    try:
        system = platform.system()
        if system == "Linux":
            for line in open("/proc/cpuinfo"):
                if line.lower().startswith(("model name", "hardware", "cpu model")):
                    return line.split(":", 1)[1].strip()
            m = re.search(r"Model name:\s*(.+)", subprocess.run(["lscpu"], capture_output=True, text=True).stdout)
            if m:
                return m.group(1).strip()
        elif system == "Darwin":
            return subprocess.run(["sysctl", "-n", "machdep.cpu.brand_string"], capture_output=True, text=True).stdout.strip()
    except OSError:
        pass
    return platform.processor() or platform.machine()


def short_cpu(name):
    name = re.sub(r"\((R|TM|r|tm)\)", "", name)
    name = re.sub(r"@.*$", "", name)
    name = re.sub(r"\b(CPU|Processor)\b", "", name)
    return re.sub(r"\s+", " ", name).strip()


def os_name():
    system = platform.system()
    if system == "Linux":
        return "Linux (WSL2)" if "microsoft" in platform.release().lower() else "Linux"
    if system == "Darwin":
        return "macOS " + platform.mac_ver()[0]
    return f"{system} {platform.release()}"


def cache_value(build, key):
    for line in open(build / "CMakeCache.txt"):
        if line.startswith(key + ":"):
            return line.split("=", 1)[1].strip()
    return ""


def compiler_name(build):
    exe = cache_value(build, "CMAKE_CXX_COMPILER")
    try:
        first = subprocess.run([exe, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    except (OSError, IndexError):
        return Path(exe).name
    m = re.search(r"(Apple clang|clang|GCC|gcc|g\+\+)[^\d]*(\d+\.\d+(?:\.\d+)?)", first)
    return f"{m.group(1).replace('g++', 'gcc').replace('GCC', 'gcc')} {m.group(2)}" if m else first


def build_flags(build):
    flags = f"{cache_value(build, 'CMAKE_CXX_FLAGS_RELEASE')} {cache_value(build, 'CMAKE_CXX_FLAGS')}".replace("-DNDEBUG", "")
    return re.sub(r"\s+", " ", flags).strip()


def git_commit():
    r = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], capture_output=True, text=True)
    return r.stdout.strip() if r.returncode == 0 else ""


# ---------------------------------------------------------------------------------------------
# build and benchmark
# ---------------------------------------------------------------------------------------------
def native_flag():
    return "-mcpu=native" if platform.system() == "Darwin" and platform.machine() == "arm64" else "-march=native"


def build(args, name, flags, with_python):
    path = args.build_dir / name
    cmd = ["cmake", "-S", ROOT, "-B", path, "-DCMAKE_BUILD_TYPE=Release", "-DSIPM_ENABLE_BENCH=ON", f"-DPython_EXECUTABLE={sys.executable}"]
    if flags:
        cmd.append(f"-DCMAKE_CXX_FLAGS={flags}")
    targets = ["events"]
    if with_python:
        cmd.append("-DSIPM_BUILD_PYTHON=ON")
        targets.append("SiPM")
    run(cmd)
    run(["cmake", "--build", path, "-j", args.jobs, "--target", *targets])
    return path


def run_benchmark(build_dir, raw_json, repetitions, min_time):
    exe = build_dir / "bench" / "events"
    base = [exe, f"--benchmark_repetitions={repetitions}", "--benchmark_report_aggregates_only=true",
            f"--benchmark_out={raw_json}", "--benchmark_out_format=json"]
    # older Google Benchmark versions do not know the "s" suffix or the random interleaving
    for extra in (["--benchmark_enable_random_interleaving=true", f"--benchmark_min_time={min_time}s"],
                  [f"--benchmark_min_time={min_time}"], []):
        print("$", " ".join(str(c) for c in base + extra), flush=True)
        if subprocess.run([str(c) for c in base + extra], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0:
            return
    raise RuntimeError(f"{exe} failed")


def trim(raw_json, build_dir, repetitions):
    """The results of a benchmark run plus the description of the machine, as saved in docs/data."""
    raw = json.load(open(raw_json))
    agg = {}
    for b in raw["benchmarks"]:
        agg.setdefault(b["run_name"].replace(" ", ""), {})[b["aggregate_name"]] = b
    results = {}
    for name, a in agg.items():
        results[name] = {"ns": round(a["median"]["real_time"], 1), "mean": round(a["mean"]["real_time"], 1),
                         "std": round(a["stddev"]["real_time"], 1), "cv": round(a["cv"]["real_time"], 4),
                         "pe": round(a["median"].get("pe", 0), 3)}
    cpu = cpu_name()
    return {"cpu": cpu, "cpu_short": short_cpu(cpu), "threads": os.cpu_count(), "os": os_name(),
            "compiler": compiler_name(build_dir), "flags": build_flags(build_dir), "repetitions": repetitions,
            "date": datetime.date.today().isoformat(), "commit": git_commit(),
            "unit": "ns per event; median, mean and standard deviation of the repetitions; pe = mean number of hit cells per event",
            "results": results}


# ---------------------------------------------------------------------------------------------
# README
# ---------------------------------------------------------------------------------------------
def approx(us):
    """A time in µs rounded for a sentence: 2.2 -> 2, 31 -> 30, 0.87 -> 0.9."""
    if us < 1:
        return f"{us:.1f}"
    return f"{us:.0f}" if us < 10 else f"{5 * round(us / 5):.0f}"


def fmt(us):
    return f"{us:.2f} µs" if us < 10 else f"{us:.1f} µs"


def scan_fit(r, mode):
    ns = np.array(SCAN)
    t = np.array([r[f"Scan/{mode}/{n}"]["mean"] for n in SCAN]) / 1000          # µs
    pe = np.array([r[f"Scan/{mode}/{n}"]["pe"] for n in SCAN])                  # hit cells per event
    sel = (ns >= 16) & (ns <= 256)
    c, a = np.polyfit(pe[sel], t[sel], 1, w=1 / t[sel])                         # T = a + c * n_pe
    dev = np.max(np.abs(t[sel] - (a + c * pe[sel])) / t[sel]) * 100
    per = t / pe * 1000                                                         # ns per hit cell
    return {"c": c * 1000, "a": max(a, 0.0), "dev": dev, "per_1k": per[SCAN.index(1024)], "per_hi": per[-1],
            "pe_per_photon": pe[SCAN.index(1024)] / 1024}


def build_performance(d, n, meta):
    """The filled template of the Performance section."""
    us = lambda r, k: r[k]["ns"] / 1000
    fit = {m: scan_fit(d, m) for m in ("Full", "HitsOnly")}
    rows = [("Empty event", "Events/Empty"), ("25 photons", "Events/Photons25"), ("1000 photons", "Events/Photons1000")]
    native = meta.get("native")

    head = "| Event | `runEvent()` | `runEvent(false)` |" + (" `runEvent()`, native | `runEvent(false)`, native |" if n else "")
    table = head + "\n|---|---|---|" + ("---|---|" if n else "") + "\n"
    for name, k in rows:
        table += f"| {name} | {fmt(us(d, k + '/Full'))} | {fmt(us(d, k + '/HitsOnly'))} ({us(d, k + '/Full') / us(d, k + '/HitsOnly'):.1f}× faster) |"
        table += (f" {fmt(us(n, k + '/Full'))} | {fmt(us(n, k + '/HitsOnly'))} |" if n else "") + "\n"

    quoted = [k for k in d if k.startswith(("Events/", "PdeBench/"))]
    spread = max(100 * max(d[k]["cv"] for k in quoted), 100 * max(n[k]["cv"] for k in quoted) if n else 0)
    note = "."
    if n:
        ratio = [us(n, k + "/Full") / us(d, k + "/Full") for _, k in rows]
        note = (f"; the last two columns use `{native['flags'].split()[-1]}` instead. With the native flags a complete empty event takes "
                f"{ratio[0]:.2f}× the time of the default build, a 25-photon event {ratio[1]:.2f}× and a 1000-photon event {ratio[2]:.2f}×.")

    hi_ratio = fit["Full"]["per_hi"] / fit["Full"]["c"]
    if hi_ratio > 1.15:
        high = ("Above a few hundred photoelectrons the cost per hit cell grows slowly (the dashed extrapolation of the fit falls below the "
                f"points): at 1000 photons it is {fit['Full']['per_1k']:.0f} ns with the signal and {fit['HitsOnly']['per_1k']:.0f} ns without, "
                f"and at 16384 photons {fit['Full']['per_hi']:.0f} ns and {fit['HitsOnly']['per_hi']:.0f} ns.")
    else:
        high = ("Above a few hundred photoelectrons the cost per hit cell stays about constant: "
                f"{fit['Full']['per_1k']:.0f} ns with the signal and {fit['HitsOnly']['per_1k']:.0f} ns without at 1000 photons, "
                f"{fit['Full']['per_hi']:.0f} ns and {fit['HitsOnly']['per_hi']:.0f} ns at 16384 photons.")

    # PDE
    pde = "| Photons | PDE | Hit cells (measured) | `runEvent()` | per hit cell | `runEvent(false)` | per hit cell |\n|---|---|---|---|---|---|---|\n"
    for sz, N in (("Photons25", 25), ("Photons1000", 1000)):
        for lab, k in (("none", "NoPde"), ("flat, 42 %", "FlatPde"), ("spectral, 42 % on average", "SpectralPde")):
            F, H = d[f"PdeBench/{sz}/{k}/Full"], d[f"PdeBench/{sz}/{k}/HitsOnly"]
            pde += f"| {N} | {lab} | {F['pe']:.0f} | {fmt(F['ns'] / 1000)} | {F['ns'] / F['pe']:.0f} ns | {fmt(H['ns'] / 1000)} | {H['ns'] / H['pe']:.0f} ns |\n"
    per = lambda sz, k, mode="Full": d[f"PdeBench/{sz}/{k}/{mode}"]["ns"] / d[f"PdeBench/{sz}/{k}/{mode}"]["pe"]
    pn, pf, ps = (per("Photons1000", k) for k in ("NoPde", "FlatPde", "SpectralPde"))
    c0 = per("Photons1000", "NoPde", "HitsOnly")
    sel = {k: max(d[f"PdeBench/Photons1000/{k}/HitsOnly"]["ns"] - c0 * d[f"PdeBench/Photons1000/{k}/HitsOnly"]["pe"], 0) / 1000
           for k in ("FlatPde", "SpectralPde")}
    d25 = {k: (d[f"PdeBench/Photons25/{k}/Full"]["ns"] - d["PdeBench/Photons25/NoPde/Full"]["ns"]) / 1000 for k in ("FlatPde", "SpectralPde")}
    if pf > 1.05 * pn and ps > 1.05 * pn:
        verdict = ("It goes **up** with a PDE: every input photon pays for the selection (a random number, plus an interpolation in the "
                   "table of the spectrum for the spectral PDE) while only the detected ones become hits.")
    else:
        verdict = ("It does not go up with a PDE: the selection (a random number, plus an interpolation in the table of the spectrum for the "
                   "spectral PDE) costs little compared with the hits.")
    pde_text = (f"{verdict} With 1000 photons and the signal the cost per hit cell is {pn:.0f} ns without PDE, {pf:.0f} ns with the flat PDE "
                f"and {ps:.0f} ns with the spectral one. Taking the hits at the cost per hit cell of the no-PDE case, the selection itself costs about "
                f"{sel['FlatPde']:.0f} ns per input photon with the flat PDE and {sel['SpectralPde']:.0f} ns with the spectral PDE (an estimate from "
                f"the 1000-photon hits-only events). For 25 photons the fixed costs of the event dominate: the time changes by "
                f"{d25['FlatPde']:+.2f} µs with the flat PDE and {d25['SpectralPde']:+.2f} µs with the spectral one.")
    lower = d["PdeBench/Photons1000/FlatPde/Full"]["ns"] < d["PdeBench/Photons1000/NoPde/Full"]["ns"]
    conclusion = ("The total time of a large event is still lower with a PDE because the detected hits are what costs most." if lower
                  else "On this machine the total time of a large event is not lower with a PDE.")

    hardware = f"{meta['cpu_short']}, {meta['os']}, {meta['compiler']}"
    values = {"REPS": meta["repetitions"], "PE_PER_PHOTON": f"{fit['Full']['pe_per_photon']:.2f}", "FIT_DEV": f"{fit['Full']['dev']:.0f}",
              "C_FULL": f"{fit['Full']['c']:.1f}", "C_HITS": f"{fit['HitsOnly']['c']:.1f}",
              "A_FULL": f"{fit['Full']['a']:.2f}", "A_HITS": f"{fit['HitsOnly']['a']:.2f}", "HIGH_N": high,
              "EVENTS_TABLE": table.rstrip(), "HARDWARE": hardware, "FLAGS": meta["flags"], "NATIVE_NOTE": note,
              "SPREAD": f"{math.ceil(spread):.0f}", "SKIP_EMPTY": f"{us(d, 'Events/Empty/Full') / us(d, 'Events/Empty/HitsOnly'):.0f}",
              "SKIP_25": f"{us(d, 'Events/Photons25/Full') / us(d, 'Events/Photons25/HitsOnly'):.1f}",
              "PDE_TEXT": pde_text, "PDE_TABLE": pde.rstrip(), "PDE_CONCLUSION": conclusion}
    text = (DOCS / "performance.md.in").read_text()
    for key, value in values.items():
        text = text.replace(f"@@{key}@@", str(value))
    left = re.findall(r"@@\w+@@", text)
    if left:
        raise RuntimeError(f"placeholders not filled: {left}")
    return text.rstrip() + "\n"


def update_readme(d, n, meta):
    path = ROOT / "README.md"
    s = path.read_text()
    section = build_performance(d, n, meta)
    s, k = re.subn(r"(<!-- BEGIN:performance[^>]*-->\n).*?(<!-- END:performance -->)", lambda m: m.group(1) + section + "\n" + m.group(2), s, flags=re.S)
    if k != 1:
        raise RuntimeError("README.md: markers BEGIN:performance / END:performance not found")
    us = lambda k_: d[k_]["ns"] / 1000
    sentence = f"A typical 25-photon event takes about {approx(us('Events/Photons25/Full'))} µs, a 1000-photon event about {approx(us('Events/Photons1000/Full'))} µs"
    s, k = re.subn(r"<!-- AUTO:typical -->.*?<!-- /AUTO:typical -->", f"<!-- AUTO:typical -->{sentence}<!-- /AUTO:typical -->", s, flags=re.S)
    if k != 1:
        raise RuntimeError("README.md: marker AUTO:typical not found")
    path.write_text(s)


# ---------------------------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0], formatter_class=argparse.RawDescriptionHelpFormatter, epilog=__doc__)
    ap.add_argument("--repetitions", type=int, default=25, help="repetitions of every benchmark (default 25)")
    ap.add_argument("--min-time", type=float, default=0.3, help="minimum time of a benchmark repetition in s (default 0.3)")
    ap.add_argument("--build-dir", type=Path, default=ROOT / "build-readme", help="where to build (default build-readme)")
    ap.add_argument("--jobs", default=str(os.cpu_count() or 4), help="parallel build jobs")
    ap.add_argument("--quick", action="store_true", help="smoke test: 3 short repetitions")
    ap.add_argument("--no-native", action="store_true", help="skip the build with the native flags")
    ap.add_argument("--skip-build", action="store_true", help="use the existing builds in --build-dir")
    ap.add_argument("--skip-bench", action="store_true", help="do not run the benchmarks, use docs/data")
    ap.add_argument("--skip-figures", action="store_true")
    ap.add_argument("--skip-check", action="store_true", help="do not run docs/check_readme.py")
    args = ap.parse_args()
    args.build_dir = args.build_dir.resolve()
    if args.quick:
        args.repetitions, args.min_time = 3, 0.05

    global np
    try:
        import numpy as np  # noqa: F811
        import matplotlib, mplhep  # noqa: F401
    except ImportError as e:
        sys.exit(f"missing Python package ({e.name}): pip install -r docs/requirements.txt")

    default_build = args.build_dir / "default"
    native_build = args.build_dir / "native"
    DATA.mkdir(exist_ok=True)

    if not args.skip_build:
        say("building with the default flags")
        build(args, "default", "", True)
        if not args.no_native:
            say("building with the native flags")
            try:
                build(args, "native", f"-ffast-math {native_flag()}", False)
            except subprocess.CalledProcessError:
                print("the build with the native flags failed: the native columns are left out")
                args.no_native = True
    elif not default_build.exists():
        sys.exit(f"--skip-build: {default_build} does not exist")

    if not args.skip_bench:
        print("\nBenchmarks are timing measurements: do not run anything else on this machine until they finish.")
        for kind, bdir in (("default", default_build), ("native", native_build)):
            if kind == "native" and (args.no_native or not (bdir / "bench" / "events").exists()):
                (DATA / "bench_native.json").unlink(missing_ok=True)
                continue
            say(f"running bench/events, {kind} build, {args.repetitions} repetitions (a few minutes)")
            raw = args.build_dir / f"bench_{kind}_raw.json"
            run_benchmark(bdir, raw, args.repetitions, args.min_time)
            (DATA / f"bench_{kind}.json").write_text(json.dumps(trim(raw, bdir, args.repetitions), indent=1) + "\n")

    d_meta = json.load(open(DATA / "bench_default.json"))
    n_path = DATA / "bench_native.json"
    n_meta = json.load(open(n_path)) if n_path.exists() else None
    meta = {"cpu_short": d_meta.get("cpu_short") or short_cpu(d_meta.get("cpu", "")), "os": d_meta.get("os", ""),
            "compiler": d_meta.get("compiler", ""), "flags": d_meta.get("flags", ""), "repetitions": d_meta.get("repetitions", "?")}
    if n_meta:
        meta["native"] = n_meta

    if not args.skip_figures:
        say("regenerating the figures")
        env = dict(os.environ, PYTHONPATH=str(default_build) + os.pathsep + os.environ.get("PYTHONPATH", ""))
        run([sys.executable, DOCS / "make_figures.py"], env=env)

    say("updating README.md")
    update_readme(d_meta["results"], n_meta["results"] if n_meta else None, meta)

    if not args.skip_check:
        say("checking the code of README.md")
        env = dict(os.environ, PYTHONPATH=str(default_build) + os.pathsep + os.environ.get("PYTHONPATH", ""))
        run([sys.executable, DOCS / "check_readme.py", default_build], env=env)

    print(f"\nDone: README.md, images/ and docs/data/ updated for {meta['cpu_short']}. Review with: git diff --stat")


if __name__ == "__main__":
    main()
