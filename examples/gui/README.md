# SimSiPM interactive simulator

A real-time GUI that runs the SiPM simulation continuously. It shows the signals
on an oscilloscope and fills peak / integral / ToT / ToA histograms. It can also
show the cell matrix of the last event.

Built with [Dear ImGui](https://github.com/ocornut/imgui) and
[ImPlot](https://github.com/epezent/implot) on GLFW + OpenGL 3.

## Build and run

```sh
cmake -B build -DSIPM_BUILD_GUI=ON
cmake --build build -j
./build/examples/gui/sipm_gui
```

CMake fetches GLFW (unless it is installed on the system), Dear ImGui and ImPlot.
On Linux you need the OpenGL and X11 development headers.

## How it works

```
generator ──► FIFO (photon events) ──► N × SiPMSensor workers ──► FIFO (results) ──► GUI
```

- **Generator thread.** It builds the acquisition windows (see *Event
  timing* below). For each source event it draws a number of photons, then an
  arrival time for each photon. With spectrum PDE it also draws a wavelength.
  It blocks when the first FIFO (1024 batches) is full. That FIFO is normally
  full, which just means generation is faster than simulation.
- **Worker threads.** Each worker owns its own `SiPMSensor` and runs
  `resetState` / `addPhotons` / `runEvent` on each event. It then extracts
  peak, integral, ToT and ToA for every event. Full waveform and hits are kept
  only for the two events the scope can show: the newest event above the
  trigger level and the newest event. If the second FIFO (1024 batches) is
  full because the GUI is behind, the batch is **dropped before being
  simulated**, so no work is spent on events nobody will see.
- **GUI.** Each frame it drains the second FIFO and fills the histograms with
  every event it receives. It shows one event on the scope, chosen by the
  trigger settings and the update speed.

So the threads are: one generator, N simulation workers and the GUI thread.
N defaults to the number of cores minus two, which is also the maximum.

The status bar shows these rates:
- *Generated* and *Simulated* events per second.
- *Displayed*: events actually shown on the oscilloscope.
- *Dropped*: events skipped because the GUI was behind.

Every settings change is versioned. Work produced with old settings is
discarded wherever it is in the pipeline, and the histograms restart.

## Controls

- **Numeric fields apply on Enter.** Checkboxes and drop-downs apply
  immediately.
- **Oscilloscope.**
  - Trigger can be *Auto* (free-runs if nothing crosses the level),
    *Normal* or *Single* (captures one event; *Arm* to re-arm).
  - Update speed: Max / 60 Hz / 5 Hz / 1 Hz / On click (*Acquire*).
  - Persistence overlays the last N traces.
  - The integration window is the **gate** trace below the signal. Drag its
    edges to resize it and its body to move it. Values snap to the sampling
    time and apply on release. The analysis threshold (red) is also
    draggable and applies on release. The trigger level (green) applies
    immediately.
  - The time axis can't be zoomed or panned outside 0 to the signal length.
- **Bottom view.** Pick the peak, integral, ToT or ToA histogram, or the cell
  matrix. *below threshold* is the percentage of events with no sample above
  the analysis threshold inside the gate, which are not histogrammed. Matrix
  cells are coloured by the type of their first hit. Hover a cell for details.

## Export, references and settings

- **PNG export.** The *PNG* button next to each plot (scope, histogram, cell
  matrix) saves it with a white, printer-friendly background. For one frame
  the plot is redrawn in a light style, and that screen region is written to
  the file (uncompressed PNG, no extra dependencies).
- **Histograms: counts or rate.** The drop-down next to *Log Y* switches
  between raw *Counts* and *Rate*. In *Rate* mode, each bin is shown as
  counts divided by the number
  of received events (including those below threshold) and by the bin width,
  times the window rate. So runs with different statistics, binning or timing
  can be compared directly.
  - The window rate is the source rate reduced by the trigger dead time:
    R / (1 + R·L) for Poisson.
  - With *Independent* timing there is no time base, so the axis shows the
    fraction of events per unit instead.
- **Freeze.** *Freeze* stores the current histogram as a reference ("Ref N",
  named after the SiPM model and source). References are drawn as
  semi-transparent outlines and kept when parameters change. Click a legend
  entry to hide one, and use *Clear refs* to remove the references of the
  current view.
- **CSV.** *CSV* in the histogram toolbar saves the current histogram.
  - A commented header gives the quantity, SiPM/source, entries, events
    below threshold, window rate, bin width and the rate definition.
  - Then the columns `bin_low,bin_high,counts,rate`.
- **Settings.** *Save settings* / *Load settings* write and read a JSON file
  with:
  - every simulation parameter, including the PDE curve;
  - the SiPM model, crystal and size;
  - file paths;
  - the scope and histogram options.

  Referenced files are reloaded when the settings are loaded.

## Parameter scans

*Parameter scan...* (under the settings buttons) opens the scan window. A
scan starts from the current settings (SiPM model, source, everything) and
varies one or more numeric parameters, the *axes*. Each axis has a start, an
end, a number of steps and a linear or logarithmic scale.

- **Matrix:** every combination of the axis values, with the last axis
  varying fastest. For example, 3 × 2 axes give 6 steps.
- **Linked:** all axes advance together. The first axis sets the number of
  steps, and the others follow it over their own range, so they are scanned
  as a function of the first one.

At each step the scan:
1. applies the values;
2. waits for at least *Events per step* histogram events;
3. freezes the four histograms as references ("S3: XT [%]=7.5, Mean
   photons=10");
4. optionally saves them as CSV, and the waveforms of the step (exactly
   *Events per step*, at the resolution set in *Waveform recording*).

While a scan runs, its settings are locked. At the end, or on *Abort*, the
starting settings are restored.

Every scan creates a new folder `<output folder>/scan_<date>_<time>/` with:

```
scan.json                  scan definition, base settings, and per step: values, events,
                           duration and file names (relative to the folder); rewritten after
                           every step, "status" = running / complete / aborted
step_0000_settings.json    settings of the step (can be loaded in the GUI)
step_0000_peak.csv         histograms (peak, integral, tot, toa), if enabled
step_0000.sipmw            waveforms, if enabled
...
```

```python
import json, os
from read_waveforms import read          # tools/read_waveforms.py
folder = "scans/scan_20261019_181032"
scan = json.load(open(os.path.join(folder, "scan.json")))
for step in scan["steps"]:
    print(step["values"], step["files"].get("peak"))
    if "waveforms" in step["files"]:
        header, ids, n_sources, waveforms = read(os.path.join(folder, step["files"]["waveforms"]))
```

## Waveform recording

*Waveform recording* (bottom of the left panel) writes every simulated window
to a compressed binary file (`.sipmw`).
- **Independent of the GUI.** The workers encode the waveforms as they
  simulate them, in parallel, and a dedicated thread writes the blocks. While
  recording, events are never dropped because of the GUI: only the scope and
  histogram updates are. If the disk is slower, the simulation slows down
  instead of losing waveforms.
- **Waveforms:** the number to record, after which the file is closed while
  the GUI keeps running. With *0*, it records until *Stop recording*.
- **Resolution:** the quantisation step of the stored samples (default 0.001
  p.e., maximum error half a step). This sets the balance between file size
  and fidelity.
- **Settings changes:** changing a simulation setting ends the recording,
  because the file header describes a single configuration.

Format (also written in the `encoding` section of each file header):

```
"SIPMWAV1", uint32 headerSize, JSON header (settings, run info, encoding)
blocks:  uint32 nEvents, uint32 rawSize, uint32 dataSize, zlib data
end:     uint32 0, uint32 summarySize, JSON {"events", "seconds"}
event (inside the inflated block): varint id, varint nSources, varint nSamples,
         zigzag varints of q[0] and q[i] - q[i-1];  sample = q * lsb_pe
```

How it is compressed:
- **Encoding:** samples are quantised, and successive differences are stored
  as variable-length integers (noise-like differences take about one byte).
- **Deflate:** each block of 32 waveforms is then compressed with zlib at its
  fastest level, so encoding stays parallel in the workers and writing is
  just a file append.

With the default noise, at 0.001 p.e. a 500-sample waveform takes about
1.1 bytes per sample (3.5× smaller than float32). At 0.01 p.e. it takes 0.7
bytes per sample (6×). On a 16-core machine about 450k waveforms/s are
written.

Blocks are written in completion order. Each waveform carries its event id,
which gives the original order.
[`tools/read_waveforms.py`](tools/read_waveforms.py) reads a file into numpy
arrays sorted by id. Run it on a file for a summary (`--plot N` plots some
waveforms).

## SiPM models

*Sensor → Model* fills the sensor fields from a commercial SiPM datasheet, at
the datasheet overvoltage: size, cell pitch, DCR, crosstalk, afterpulsing,
rise/fall/recovery times and the PDE curve (PDE switches to *Spectrum*).
*Custom* keeps the fields as you set them. Editing any field set by a model
turns the selection back into *Custom*. CCGV and SNR are not datasheet values
and are left as they are.

| Family | Models | Datasheet |
|---|---|---|
| Hamamatsu S13360 | -1325/1350/1375, -3025/3050/3075, -6025/6050/6075 CS | KAPD1052E |
| Hamamatsu S14160 HS | -3050HS, -4050HS, -6050HS | KAPD1064E |
| Hamamatsu S14160 PS | -1310/3010/6010PS, -1315/3015/6015PS | KAPD1070E |
| Broadcom (FBK NUV-HD) | AFBR-S4N33C013, AFBR-S4N44C013 at 3 V and 7 V overvoltage | AFBR-S4N33C013-DS, AFBR-S4N44C013-DS |
| Broadcom (FBK NUV-MT) | AFBR-S4N66P024M at 12 V | AFBR-S4N66P024M-DS |

The data are in [`data/sipm_models.json`](data/sipm_models.json).
- **PDE curves** were digitised from the datasheet plots (about ±1–2% absolute).
- **Estimated values:** some quantities are not in the datasheets, so typical
  values are used. These are the Hamamatsu fall/recovery times and
  afterpulsing, and the DCR of the S14160 HS devices, computed from their dark
  current as I_D / (q·gain·(1 + crosstalk)). Each model lists these in its
  `estimated` field, and the GUI shows them under the model.
- **Broadcom crosstalk** values are the datasheet totals, so they may include
  delayed crosstalk.

## Event timing

- **Independent.** One source event per window, no pile-up.
- **Poisson / Periodic.** The source emits a stream of events at *Rate*, with
  exponential or fixed intervals.
  - Each window is triggered by an event, and the *Time offset* is the
    pre-trigger.
  - Any other event whose light overlaps the window is simulated in it too,
    so pulses can interact and pile up at high rates.
  - The next window can open only after the current one ends (dead time =
    window length).

Each window is simulated together with a stretch of history before it:
5 × the longest of the fall time, slow fall time and recovery time, capped at
5 µs. That history is then cropped away. So tails of earlier pulses, cells
still recharging and late afterpulses all reach the visible window.

On the scope, *sources* is the number of events with light in the window.

## File formats

- **PDE spectrum.** Two columns: wavelength in nm, PDE in [0, 1]. Header lines
  are skipped. Example: [`data/pde_example.csv`](data/pde_example.csv).
- **Photon number histogram** (Photons → Histogram file). Comma separated
  relative counts (bin 0 first), with lines starting with `#` ignored. An event
  takes the value *i* of a random bin and generates *i* × *Scale* photons, with
  stochastic rounding. With a scale of 1, bin *i* is simply *i* photons.
  Example: [`data/nphotons_example.csv`](data/nphotons_example.csv).
- **Gamma lines** (Photons → Gamma source). Rows of `energy_keV,
  intensity_pct`, where the intensity is the emission probability per decay.
  Lines starting with `#` are ignored, and extra columns too.

## Gamma sources

*Photons → Gamma source* simulates a radioactive source seen by a
scintillator crystal read out by the SiPM. You pick four things:

- **Source**: a nuclide, or *Custom file* (a list of gamma lines).
- **Crystal**: the scintillator material.
- **Size**: the crystal size preset.
- **Light yield scale**: multiplies the material light yield. Use it to
  account for light collection and coupling onto the SiPM.

Everything else is fixed by the presets.

### Materials

Values are typical datasheet and literature figures and are approximate:
[`data/scintillators.json`](data/scintillators.json).

| Crystal | ph/keV | Decay (ns) | Emission (nm) | Intrinsic FWHM @662 keV |
|---|---|---|---|---|
| NaI(Tl) | 38 | 250 | 415 | 5.5% |
| CsI(Tl) | 54 | 680 (64%), 3340 | 550 | 5.0% |
| CsI (undoped) | 2.3 | 6 (60%), 35 | 315 | 3.0% |
| LYSO(Ce) | 30 | 40 | 420 | 6.0% |
| BGO | 8.2 | 300 (90%), 60 | 480 | 6.0% |
| PbWO4 | 0.2 | 10 (80%), 30 | 420 | 3.0% |
| LaBr3(Ce) | 63 | 16 | 380 | 2.0% |
| CeBr3 | 60 | 17 | 375 | 3.0% |
| GAGG(Ce) | 46 | 88 (91%), 258 | 520 | 4.5% |
| Plastic (EJ-200) | 10 | 2.1 | 425 | 10% |

- **Resolution:** the FWHM is only the intrinsic, non-statistical part, and
  scales as √E. Photon statistics, PDE and SiPM effects are added by the
  simulation.
- **Photon times:** rise time plus one of the decay components. These replace
  the *Time profile* controls; *Time offset* still applies.
- **Wavelengths:** drawn from the emission spectrum (Gaussian approximation).
  They are only generated with *Spectrum* PDE, where they combine with the PDE
  curve.

### Sizes

The source sits on the crystal axis, 10 mm from the entrance face.

| Size | Crystal |
|---|---|
| Small | 6 × 6 × 6 mm³ box |
| Medium | Ø1″ × 1″ cylinder |
| Large | Ø2″ × 2″ cylinder |

Size sets all geometric effects: photopeak / Compton ratio, multiple
scattering, X-ray and annihilation escape, and coincidence summing. For
Cs-137 in NaI(Tl), the photopeak-to-total ratio is 0.10 (small), 0.25
(medium) and 0.39 (large). The usual reference values for 1″×1″ and 2″×2″
NaI are about 0.25 and 0.4.

### Precomputed spectra

At build time, `sipm_gui_spectra` ([`tools/make_spectra.cpp`](tools/make_spectra.cpp))
runs a Monte Carlo for every source × material × size. It records 200k
detected decays each, with the deposited energy in 1 keV bins, and takes
about 25 s. The GUI then only draws an energy from the selected spectrum for
each event. The number of photons is Poisson(E × light yield × scale).

The Monte Carlo:
- Tracks every photon of each decay, including back-to-back annihilation
  pairs. True coincidence summing (e.g. the Co-60 2505 keV peak) and escape
  of annihilation photons therefore come out naturally.
- Uses NIST XCOM photon cross sections per element:
  [`data/xcom`](data/xcom), downloaded by
  [`tools/fetch_xcom.py`](tools/fetch_xcom.py).
- Models photoelectric absorption with K X-ray fluorescence and escape,
  Klein–Nishina Compton scattering, and pair production.
- Ignores: electron transport (electrons deposit locally), bremsstrahlung,
  coherent scattering, angular correlations between gammas, and materials
  around the crystal (no backscatter peak). Intrinsic radioactivity, such as
  Lu-176 in LYSO, is not included.

A custom line file goes through the same Monte Carlo once, when it is loaded
(50k decays). The result is cached.

The emission lines of the 12 nuclides (gamma, annihilation and K X-rays with
E ≥ 20 keV and intensity ≥ 0.1%) come from the LNHB/DDEP Nucléide-Lara
library (https://www.lnhb.fr/nuclides/).
[`data/nuclides/make_profiles.py`](data/nuclides/make_profiles.py) downloads
them again.
