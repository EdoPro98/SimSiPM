#!/usr/bin/env python3
"""Reader for the waveform dumps written by the SimSiPM GUI (*.sipmw).

File layout (little endian), also described in the "encoding" section of the
JSON header of every file:

    "SIPMWAV1"                               magic, 8 bytes
    uint32 headerSize, header                JSON: settings, run info, encoding
    repeated blocks:
        uint32 nEvents (> 0), uint32 rawSize, uint32 dataSize, data
        data = zlib stream; inflated it gives rawSize bytes made of, per waveform:
            varint id, varint nSources, varint nSamples,
            nSamples zigzag varints: q[0], q[1]-q[0], q[2]-q[1], ...
        sample[i] = q[i] * lsb_pe   (amplitude in photoelectrons)
    end marker: uint32 0, uint32 summarySize, summary JSON {"events", "seconds"}

varint: unsigned LEB128. zigzag: d = (u >> 1) ^ -(u & 1).
Blocks are in completion order (parallel writers): waveforms are sorted by id here.

Usage:
    python3 read_waveforms.py run.sipmw            # summary
    python3 read_waveforms.py run.sipmw --plot 5   # also plot the first waveforms

From Python:
    from read_waveforms import read
    header, ids, n_sources, waveforms = read("run.sipmw")   # waveforms: (n, samples) float32
"""

import json
import struct
import sys
import zlib

import numpy as np


def _varints(buf):
    """Decodes a byte buffer made only of LEB128 varints (vectorised)."""
    b = np.frombuffer(buf, dtype=np.uint8)
    ends = np.flatnonzero(b < 0x80)
    starts = np.empty_like(ends)
    starts[0] = 0
    starts[1:] = ends[:-1] + 1
    lengths = ends - starts + 1
    out = np.zeros(len(ends), dtype=np.uint64)
    for k in range(int(lengths.max())):
        sel = lengths > k
        out[sel] |= (b[starts[sel] + k] & 0x7F).astype(np.uint64) << np.uint64(7 * k)
    return out


def read(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"SIPMWAV1":
        raise ValueError("not a SimSiPM waveform file")
    (hsize,) = struct.unpack_from("<I", data, 8)
    header = json.loads(data[12:12 + hsize])
    lsb = header["encoding"]["lsb_pe"]
    pos = 12 + hsize
    ids, nsrc, waves, summary = [], [], [], None
    while pos < len(data):
        (n,) = struct.unpack_from("<I", data, pos)
        if n == 0:
            (ssize,) = struct.unpack_from("<I", data, pos + 4)
            summary = json.loads(data[pos + 8:pos + 8 + ssize])
            break
        raw_size, size = struct.unpack_from("<II", data, pos + 4)
        raw = zlib.decompress(data[pos + 12:pos + 12 + size])
        assert len(raw) == raw_size, "corrupted block"
        v = _varints(raw)
        i = 0
        for _ in range(n):
            ids.append(int(v[i]))
            nsrc.append(int(v[i + 1]))
            ns = int(v[i + 2])
            u = v[i + 3:i + 3 + ns].astype(np.int64)
            d = (u >> 1) ^ -(u & 1)
            waves.append((np.cumsum(d) * lsb).astype(np.float32))
            i += 3 + ns
        pos += 12 + size
    order = np.argsort(ids)
    header["summary"] = summary
    return header, np.asarray(ids)[order], np.asarray(nsrc)[order], np.stack(waves)[order] if waves else np.empty((0, 0))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    header, ids, nsrc, w = read(sys.argv[1])
    run = header["run"]
    print(f"{header['format']} v{header['version']}, created {header['created']}")
    print(f"SiPM {run['sipm_model']}, timing {run['event_timing']}, sampling {run['sampling_ns']} ns, "
          f"{run['samples_per_waveform']} samples, lsb {header['encoding']['lsb_pe']} p.e.")
    print(f"{len(ids)} waveforms (end marker: {header['summary']}), ids {ids.min() if len(ids) else '-'}.."
          f"{ids.max() if len(ids) else '-'}, sources per window mean {nsrc.mean() if len(nsrc) else 0:.3f}")
    if len(w):
        print(f"amplitude: max mean {w.max(axis=1).mean():.3f} p.e., baseline rms (first 50 samples) "
              f"{w[:, :50].std():.4f} p.e.")
    if "--plot" in sys.argv:
        import matplotlib.pyplot as plt
        n = int(sys.argv[sys.argv.index("--plot") + 1])
        t = np.arange(w.shape[1]) * run["sampling_ns"]
        for k in range(min(n, len(w))):
            plt.plot(t, w[k], lw=0.8, label=f"id {ids[k]}")
        plt.xlabel("Time [ns]")
        plt.ylabel("Amplitude [p.e.]")
        plt.legend()
        plt.show()
    return 0


if __name__ == "__main__":
    sys.exit(main())
