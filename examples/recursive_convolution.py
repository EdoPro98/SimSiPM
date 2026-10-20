"""Visual explanation of the recursive convolution used by SiPMSensor::generateSignal.

Both methods are re-implemented here in float32, operation by operation as in
src/SiPMSensor.cpp, so their difference is the same rounding noise the C++ has.
If the SiPM module is importable, the real C++ (recursive) output is also checked
against the direct sum built from the hits the library reports.

Usage: python examples/recursive_convolution.py [output_dir]   (default: ./recursive_convolution_plots)
"""
import os
import sys

import matplotlib.pyplot as plt
import numpy as np

K = 8  # kBlock in SiPMSensor.cpp

# Illustration parameters (ns). A slow rise makes the rising component visible.
SAMPLING = 1.0
SIGNAL_LENGTH = 300
RISE, FALL_FAST, FALL_SLOW, SLOW_FRACTION = 5.0, 40.0, 150.0, 0.3
GAIN = 1.0
N = int(SIGNAL_LENGTH / SAMPLING)

HIT_TIMES = np.array([20, 23, 60, 61, 61, 130, 200])
HIT_AMPS = np.array([1.0, 0.8, 1.0, 0.6, 1.1, 0.9, 1.0], dtype=np.float32)


def components():
    """(tau, weight) of each exponential: g[k] = norm * sum_c w_c * exp(-k*dt/tau_c)."""
    return [(FALL_FAST, 1 - SLOW_FRACTION), (RISE, -1.0), (FALL_SLOW, SLOW_FRACTION)]


def signal_shape():
    """SiPMSensor::signalShape(): sampled pulse normalised to peak = gain."""
    i = np.arange(N, dtype=np.float32)
    g = np.zeros(N, dtype=np.float32)
    for tau, w in components():
        g += np.float32(w) * np.exp(-i / np.float32(tau / SAMPLING))
    norm = np.float32(GAIN) / g.max()  # m_SignalNorm
    return g * norm, norm


def direct(times, amps, shape):
    """Old method: add one shifted copy of the shape per hit. Cost ~ n * N."""
    s = np.zeros(N, dtype=np.float32)
    for t, a in zip(times, amps):
        s[t:] += shape[: N - t] * a
    return s


def first_order(times, amps, d):
    """y[i] = d*y[i-1] + x[i]: the idea, but every sample waits for the previous one."""
    x = np.zeros(N, dtype=np.float32)
    np.add.at(x, times, amps)
    y = np.zeros(N, dtype=np.float32)
    for i in range(N):
        y[i] = (y[i - 1] * d if i else 0) + x[i]
    return x, y


def scatter_z(times, amps, d):
    """z[t+j] += a*d^j for j < K: each hit spread over K taps (padded like the C++ z)."""
    pw = (np.float32(d) ** np.arange(K)).astype(np.float32)
    z = np.zeros(N + K, dtype=np.float32)
    for t, a in zip(times, amps):
        z[t : t + K] += a * pw
    return z


def block_recursion(z, dK, upto_block=None):
    """z[i] += d^K * z[i-K]. Within one block the K samples are independent,
    so each block is ONE SIMD operation: this is what makes it vectorizable."""
    z = z.copy()
    nblocks = (N + K - 1) // K
    last = nblocks if upto_block is None else upto_block
    for b in range(1, last):
        lo, hi = b * K, min((b + 1) * K, N)
        z[lo:hi] += dK * z[lo - K : hi - K]
    return z[:N]


def recursive(times, amps, norm):
    """New method: one block recursion per exponential component. Cost ~ C * (K*n + 3N)."""
    s = np.zeros(N, dtype=np.float32)
    for tau, w in components():
        d = np.float32(np.exp(-SAMPLING / tau))
        dK = np.float32(d ** K)
        s += np.float32(w) * norm * block_recursion(scatter_z(times, amps, d), dK)
    return s


def savefig(fig, outdir, name):
    path = os.path.join(outdir, name)
    fig.tight_layout()
    fig.savefig(path, dpi=130)
    print("wrote", path)


def fig_shape(outdir, shape, norm):
    k = np.arange(N)
    fig, ax = plt.subplots(figsize=(9, 4.5))
    labels = ["fast decay  (1-f)·e^(-kΔt/τf)", "rise  -e^(-kΔt/τr)", "slow decay  f·e^(-kΔt/τs)"]
    for (tau, w), lab in zip(components(), labels):
        ax.plot(k, w * norm * np.exp(-k * SAMPLING / tau), "--", label=lab)
    ax.plot(k, shape, "k", lw=2.5, label="pulse g[k] = sum of the three")
    ax.axhline(0, color="gray", lw=0.5)
    ax.set_xlim(0, 200)
    ax.set_xlabel("sample k")
    ax.set_title("1. The pulse shape is a sum of exponentials, each one = d^k with d = e^(-Δt/τ)")
    ax.legend()
    savefig(fig, outdir, "1_shape_components.png")


def fig_direct(outdir, shape):
    fig, (ax0, ax1) = plt.subplots(2, 1, figsize=(9, 6), sharex=True)
    ax0.stem(HIT_TIMES, HIT_AMPS, basefmt=" ")
    ax0.set_ylabel("amplitude a_h")
    ax0.set_title("2. Direct method: one full-length shifted pulse per hit, then sum  (n·N work)")
    total = np.zeros(N, dtype=np.float32)
    for t, a in zip(HIT_TIMES, HIT_AMPS):
        copy = np.zeros(N, dtype=np.float32)
        copy[t:] = shape[: N - t] * a
        total += copy
        ax1.plot(copy, lw=1, alpha=0.6)
    ax1.plot(total, "k", lw=2.5, label="s[i] = Σ_h a_h·g[i - t_h]")
    ax1.set_xlabel("sample i")
    ax1.legend()
    savefig(fig, outdir, "2_direct_convolution.png")


def fig_first_order(outdir, shape, norm):
    names = ["fast", "rise", "slow"]
    x = None
    states = []
    for tau, _ in components():
        d = np.float32(np.exp(-SAMPLING / tau))
        x, y = first_order(HIT_TIMES, HIT_AMPS, d)
        states.append((d, y))

    fig, axs = plt.subplots(3, 1, figsize=(9, 9), sharex=True)
    axs[0].stem(np.nonzero(x)[0], x[x != 0], basefmt=" ")
    axs[0].set_ylabel("x[i]")
    axs[0].set_title("3. First-order recursion  y[i] = d·y[i-1] + x[i]  (one state per exponential)")
    axs[0].text(140, 1.2, "x[i] = total amplitude landing in bin i\n(the 3 hits near i=60 fall in bins 60 and 61)", fontsize=9)

    for name, (d, y) in zip(names, states):
        axs[1].step(range(N), y, where="post", label=f"y_{name}  (d = {d:.4f})")
    axs[1].annotate("+a_h: every state\njumps by the hit amplitude", xy=(200, states[0][1][200]),
                    xytext=(220, 0.6), arrowprops=dict(arrowstyle="->"))
    axs[1].annotate("×d per sample: geometric decay,\nfaster for smaller τ", xy=(100, states[0][1][100]),
                    xytext=(135, 1.9), arrowprops=dict(arrowstyle="->"))
    axs[1].set_ylabel("state y_c[i]")
    axs[1].legend()

    total = np.zeros(N, dtype=np.float32)
    for name, (tau, w), (_, y) in zip(names, components(), states):
        part = np.float32(w) * norm * y
        total += part
        axs[2].plot(part, "--", lw=1, label=f"norm·w_{name}·y_{name}  (w = {w:g})")
    axs[2].plot(total, "k", lw=3, label="sum of the weighted states")
    axs[2].plot(direct(HIT_TIMES, HIT_AMPS, shape), "r:", lw=2, label="direct method (fig. 2): identical")
    axs[2].axhline(0, color="gray", lw=0.5)
    axs[2].set_xlabel("sample i")
    axs[2].legend(fontsize=8)
    savefig(fig, outdir, "3_first_order_recursion.png")


def fig_block(outdir):
    tau, _ = components()[0]
    d = np.float32(np.exp(-SAMPLING / tau))
    dK = np.float32(d ** K)
    z = scatter_z(HIT_TIMES, HIT_AMPS, d)[:N]

    fig, axs = plt.subplots(2, 1, figsize=(9, 7))
    axs[0].bar(range(N), z, width=1.0, color="tab:orange")
    axs[0].set_xlim(0, 100)
    axs[0].set_title(f"4a. Scatter (fast component): each hit adds a·d^j to z[t+j], j < K={K}  -  O(K·n)\n"
                     "z[i] = Σ_j d^j·x[i-j]: the last K samples of history, already decayed")
    axs[0].set_ylabel("z[i]")
    for b in range(0, 100, K):
        axs[0].axvline(b - 0.5, color="gray", lw=0.4)

    # Lanes: sample i depends only on i-K, so the K samples of one block are independent
    ax = axs[1]
    lo, hi = 16, 48
    ax.set_title("4b. Block recursion z[i] += d^K·z[i-K]: arrows only jump K back,\n"
                 "so the K samples of a block update together = one SIMD instruction")
    colors = plt.cm.tab10(np.arange(K) % 10)
    for i in range(lo, hi):
        ax.scatter(i, i % K, color=colors[i % K], s=40, zorder=3)
        if i - K >= lo:
            ax.annotate("", xy=(i, i % K), xytext=(i - K, i % K), arrowprops=dict(arrowstyle="->", color=colors[i % K], lw=0.8))
    for b in range(lo, hi + 1, K):
        ax.axvline(b - 0.5, color="gray", lw=0.6)
        if b < hi:
            ax.text(b + K / 2 - 0.5, K - 0.2, f"block {b // K}", ha="center", fontsize=8)
    ax.set_yticks(range(K))
    ax.set_ylabel("SIMD lane = i mod K")
    ax.set_xlabel("sample i")
    ax.set_ylim(-0.7, K + 0.3)
    savefig(fig, outdir, "4_block_recursion.png")

    # Step by step: blocks left of the frontier hold the full state, right of it still raw stubs
    _, y_ref = first_order(HIT_TIMES, HIT_AMPS, d)
    snaps = [3, 8, 17, 26, (N + K - 1) // K]
    fig, axs = plt.subplots(len(snaps), 1, figsize=(9, 11), sharex=True, sharey=True)
    for ax, nb in zip(axs, snaps):
        part = block_recursion(scatter_z(HIT_TIMES, HIT_AMPS, d), dK, upto_block=nb)
        f = min(nb * K, N)
        ax.plot(y_ref, color="lightgray", lw=4, label="target y[i] (first-order recursion)")
        ax.plot(range(f), part[:f], "k", lw=1.5, label="processed: final state")
        ax.bar(range(f, N), part[f:], width=1.0, color="tab:orange", label="not processed yet: raw z stubs")
        ax.axvline(f - 0.5, color="tab:red", ls="--")
        ax.set_title(f"after {nb} blocks (frontier at i = {f})", fontsize=9, loc="left")
    axs[0].legend(fontsize=8, loc="upper right")
    axs[-1].set_xlabel("sample i")
    fig.suptitle("4c. Recursion step by step: each block adds d^K × (block K samples earlier) to its stubs,\n"
                 "which brings the decayed tail of all older hits forward. The end result equals the target")
    savefig(fig, outdir, "4c_block_recursion_steps.png")


def fig_compare(outdir, shape, norm):
    rng = np.random.default_rng(1)
    n_hits = 200
    times = rng.integers(0, N, n_hits)
    amps = rng.normal(1, 0.1, n_hits).astype(np.float32)
    s_dir = direct(times, amps, shape)
    s_rec = recursive(times, amps, norm)

    fig, axs = plt.subplots(3, 1, figsize=(9, 9))
    axs[0].plot(s_dir, lw=3, label="direct (old)")
    axs[0].plot(s_rec, "--", lw=1.5, label="recursive (new)")
    axs[0].set_title(f"5a. Same event, {n_hits} hits")
    axs[0].legend()

    axs[1].plot((s_rec - s_dir) / np.abs(s_dir).max())
    axs[1].set_title("5b. (recursive − direct) / peak: float32 rounding noise only, no drift")
    axs[1].set_xlabel("sample i")

    # C++ rule (SiPMSensor::generateSignal): recursive when sum_h (N - t_h) > kRecursiveCost * C * N
    n = np.arange(1, 65)
    C = len(components())
    axs[2].plot(n, n * N / 2, label="direct work Σ(N − t_h) ≈ n·N/2  (grows with hits)")
    axs[2].axhline(2 * C * N, color="tab:orange", label=f"recursive cost ≈ kRecursiveCost·C·N = 2·{C}·{N}  (flat)")
    axs[2].axvline(4 * C, color="gray", ls=":", label=f"crossover ≈ {4 * C} hits")
    axs[2].set_xlabel("number of hits n")
    axs[2].set_ylabel("cost (direct-loop units)")
    axs[2].set_title(f"5c. Which path the C++ takes, N = {N}  (kRecursiveCost = 2 was measured)")
    axs[2].legend(fontsize=8)
    savefig(fig, outdir, "5_comparison.png")
    print(f"max |recursive - direct| / peak = {np.abs(s_rec - s_dir).max() / np.abs(s_dir).max():.2e}")


# ---------------------------------------------------------------------------
# Schematic figures: every sample drawn as a square, on a toy-sized problem
# ---------------------------------------------------------------------------
SN, SK = 24, 4                      # samples and block size (C++ uses K = 8)
S_TIMES, S_AMPS = [2, 7, 9], [1.0, 0.8, 1.2]
S_TAUS, S_WEIGHTS = [6.0, 1.0], [1.0, -1.0]  # fast decay and rise, in samples


def draw_row(ax, y, values, label, start=0, vmax=1.0, ghost=(), red=(), dim=(), fmt="{:.2f}"):
    """Draw values[k] as a square at column start+k, row top at -y.
    ghost: untouched columns (dotted outline), red: highlighted columns, dim: not final yet."""
    y = -y
    for c in ghost:
        ax.add_patch(plt.Rectangle((c, y), 0.92, 0.92, fill=False, ec="0.85", ls=":"))
    for k, v in enumerate(values):
        c = start + k
        v = 0 if abs(v) < 0.005 else v
        level = 0.15 + 0.75 * min(abs(v) / vmax, 1)
        color = plt.cm.Blues(level) if v > 0 else plt.cm.Oranges(level) if v < 0 else "white"
        alpha = 0.3 if c in dim else 1
        ax.add_patch(plt.Rectangle((c, y), 0.92, 0.92, fc=color, alpha=alpha, ec="none"))
        ax.add_patch(plt.Rectangle((c, y), 0.92, 0.92, fill=False,
                                   ec="red" if c in red else "0.4", lw=2.2 if c in red else 0.6))
        if v != 0:
            ax.text(c + 0.46, y + 0.46, fmt.format(v), ha="center", va="center", fontsize=5.5, alpha=max(alpha, 0.6),
                    color="white" if level > 0.6 and alpha == 1 else "black")
    ax.text(-0.6, y + 0.46, label, ha="right", va="center", fontsize=8)


def schematic_axes(height, title):
    fig, ax = plt.subplots(figsize=(14, height))
    ax.set_xlim(-12, SN + 7)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(title, fontsize=11)
    for c in range(SN):
        ax.text(c + 0.46, 1.15, str(c), ha="center", fontsize=6, color="0.5")
    return fig, ax


def save_schematic(fig, ax, bottom, outdir, name):
    ax.set_ylim(-bottom - 0.3, 1.5)
    path = os.path.join(outdir, name)
    fig.savefig(path, dpi=150, bbox_inches="tight")
    print("wrote", path)


def sum_line(ax, y, symbol):
    ax.plot([-11, SN], [-y + 0.98, -y + 0.98], "k", lw=1)
    ax.text(-10.8, -y + 0.46, symbol, fontsize=13, va="center", ha="center")


def schematic_shape():
    k = np.arange(SN)
    g = sum(w * np.exp(-k / tau) for tau, w in zip(S_TAUS, S_WEIGHTS))
    return g / g.max()


def fig_direct_schematic(outdir):
    g = schematic_shape()
    s = np.zeros(SN)
    fig, ax = schematic_axes(5, "6. Direct method: one shifted, scaled copy of the WHOLE shape per hit, "
                                "then a column-wise sum")
    col = 11  # highlighted output sample
    draw_row(ax, 0, g, "pulse shape g[k]")
    ax.text(SN / 2, -0.75, "↓ copy, shift right by t_h, multiply by a_h", fontsize=7, color="0.4", ha="center")
    y = 2
    for h, (t, a) in enumerate(zip(S_TIMES, S_AMPS)):
        copy = a * g[: SN - t]
        s[t:] += copy
        draw_row(ax, y, copy, f"hit {h}:  {a} × g[i − {t}]", start=t, ghost=range(t), red=[col])
        ax.text(SN + 0.5, -y + 0.46, f"{SN - t} mult-adds", fontsize=7, va="center", color="0.3")
        if h:
            ax.text(-10.8, -y + 0.46, "+", fontsize=13, va="center", ha="center")
        y += 1
    y += 0.3
    sum_line(ax, y, "=")
    draw_row(ax, y, s, "signal s[i]", vmax=s.max(), red=[col])
    ax.annotate(f"s[{col}] = sum of the red column", xy=(col + 0.46, -y), xytext=(col + 3, -y - 1.2),
                fontsize=7, color="red", arrowprops=dict(arrowstyle="->", color="red"))
    ax.text(SN + 0.5, -y + 0.46, f"total {sum(SN - t for t in S_TIMES)}\n≈ n·N/2 mult-adds", fontsize=7, va="center")
    save_schematic(fig, ax, y + 1.5, outdir, "6_direct_schematic.png")
    return s


def schematic_component(tau):
    """Scatter + block recursion for one exponential; returns d, stubs, z after each block."""
    d = np.exp(-1 / tau)
    stubs = [(t, a * d ** np.arange(SK)) for t, a in zip(S_TIMES, S_AMPS)]
    z = np.zeros(SN + SK)
    for t, stub in stubs:
        z[t : t + SK] += stub
    z = z[:SN]
    snaps = [z.copy()]
    for b in range(1, SN // SK):
        z[b * SK : (b + 1) * SK] += d ** SK * z[(b - 1) * SK : b * SK]
        snaps.append(z.copy())
    return d, stubs, snaps


def fig_recursive_schematic(outdir, s_direct):
    norm = 1 / max(sum(w * np.exp(-k / tau) for tau, w in zip(S_TAUS, S_WEIGHTS)) for k in range(SN))
    d, stubs, snaps = schematic_component(S_TAUS[0])
    dK = d ** SK
    nblocks = SN // SK
    vmax = max(snaps[-1])
    fig, ax = schematic_axes(13, f"7. Recursive method, steps 1-3 shown for the fast-decay component "
                                 f"(d = e^(-Δt/τ) = {d:.3f}, block K = {SK})")

    def step(y, text):
        ax.text(-11.8, -y + 0.25, text, fontsize=9, weight="bold")

    x = np.zeros(SN)
    np.add.at(x, S_TIMES, S_AMPS)
    step(0, "Step 1  put the hit amplitudes in their time bins")
    y = 0.9
    draw_row(ax, y, x, "x[i]")

    y += 1.6
    step(y, f"Step 2  scatter: each hit writes only K = {SK} taps, already decayed:  z[t+j] += a·d^j")
    y += 0.9
    for h, (t, stub) in enumerate(stubs):
        draw_row(ax, y, stub, f"hit {h}:  {S_AMPS[h]} × d^j,  j < {SK}", start=t, ghost=range(SN))
        ax.text(SN + 0.5, -y + 0.46, f"{SK} mult-adds", fontsize=7, va="center", color="0.3")
        y += 1
    y += 0.3
    sum_line(ax, y, "Σ")
    draw_row(ax, y, snaps[0], "z[i]  (stubs summed)", vmax=vmax)

    y += 1.6
    step(y, f"Step 3  block recursion, left to right:  block b  +=  d^K × block b−1     (d^K = {dK:.3f})")
    b0, b1 = snaps[0][:SK], snaps[0][SK : 2 * SK]
    vec = lambda v: "[" + "  ".join(f"{u:.2f}" for u in v) + "]"
    ax.text(-11.8, -y - 0.55, f"e.g. block 1:  {vec(b1)}  +  {dK:.3f} × {vec(b0)}  =  {vec(b1 + dK * b0)}"
            f"      the {SK} cells are independent → one SIMD instruction", fontsize=7.5, color="red")
    y += 2.4
    top = y
    for b in range(1, nblocks):
        cur = range(b * SK, (b + 1) * SK)
        draw_row(ax, y, snaps[b], f"after block {b}", vmax=vmax, red=cur, dim=range((b + 1) * SK, SN))
        # arc from the (already final) previous block to the block being updated
        ax.annotate("", xy=((b + 0.5) * SK, -y + 0.95), xytext=((b - 0.5) * SK, -y + 0.95),
                    arrowprops=dict(arrowstyle="->", color="red", lw=1.1, connectionstyle="arc3,rad=-0.4"))
        ax.text((b - 0.5) * SK - 0.1, -y + 1.2, f"× {dK:.2f} +", fontsize=6.5, color="red", ha="right")
        ax.text(SN + 0.5, -y + 0.46, f"{SK} mult-adds\n= 1 SIMD op", fontsize=6.5, va="center", color="0.3")
        y += 1.8
    for c in range(0, SN + 1, SK):  # block boundaries
        ax.plot([c - 0.04, c - 0.04], [-y + 1.75, -top + 1.0], color="black", lw=1.6)
    ax.text(SN / 2, -y + 1.2, "faded cells: still raw stubs, not processed yet", fontsize=7, color="0.4", ha="center")
    y_fast = snaps[-1]

    _, _, snaps_r = schematic_component(S_TAUS[1])
    y_rise = snaps_r[-1]
    y += 0.6
    step(y, "Step 4  repeat steps 2-3 for every component (here also the rise), then  s = norm · Σ_c w_c · y_c")
    y += 0.9
    draw_row(ax, y, norm * S_WEIGHTS[0] * y_fast, f"+{norm:.2f} × y_fast", vmax=s_direct.max())
    y += 1
    draw_row(ax, y, norm * S_WEIGHTS[1] * y_rise, f"−{norm:.2f} × y_rise", vmax=s_direct.max())
    ax.text(-10.8, -y + 0.46, "+", fontsize=13, va="center", ha="center")
    y += 1.3
    s = norm * (S_WEIGHTS[0] * y_fast + S_WEIGHTS[1] * y_rise)
    sum_line(ax, y, "=")
    draw_row(ax, y, s, "signal s[i]", vmax=s_direct.max())
    ax.text(SN + 0.5, -y + 0.46, f"max |s − direct| = {np.abs(s - s_direct).max():.0e}\n→ same as fig. 6",
            fontsize=7, va="center", color="green")
    save_schematic(fig, ax, y + 0.5, outdir, "7_recursive_schematic.png")


def check_library():
    """Real C++ recursive output vs the direct sum of the hits the library reports."""
    try:
        import SiPM
    except ImportError:
        print("SiPM module not found, skipping library check")
        return
    p = SiPM.SiPMProperties()
    p.setSignalLength(SIGNAL_LENGTH)
    p.setSampling(SAMPLING)
    p.setRiseTime(RISE)
    p.setFallTimeFast(FALL_FAST)
    p.setFallTimeSlow(FALL_SLOW)
    p.setSlowComponentFraction(SLOW_FRACTION)
    p.setDcrOff()
    p.setXtOff()
    p.setApOff()
    p.setSnr(200)  # baseline noise negligible
    sensor = SiPM.SiPMSensor(p)
    rng = np.random.default_rng(2)
    sensor.addPhotons(list(rng.uniform(0, SIGNAL_LENGTH, 100)))
    sensor.runEvent()

    hits = sensor.hits()
    times = np.array([int(np.floor(h.time() / SAMPLING)) for h in hits])
    amps = np.array([h.amplitude() for h in hits], dtype=np.float32)
    shape, _ = signal_shape()  # library default gain is 1, same as GAIN here
    ref = direct(times, amps, shape)
    lib = np.asarray(sensor.signal().waveform())
    print(f"library (recursive C++) vs direct sum of its {len(hits)} hits: "
          f"max |diff| / peak = {np.abs(lib - ref).max() / np.abs(ref).max():.2e}")


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "recursive_convolution_plots"
    os.makedirs(outdir, exist_ok=True)
    shape, norm = signal_shape()
    fig_shape(outdir, shape, norm)
    fig_direct(outdir, shape)
    fig_first_order(outdir, shape, norm)
    fig_block(outdir)
    fig_compare(outdir, shape, norm)
    fig_recursive_schematic(outdir, fig_direct_schematic(outdir))
    check_library()


if __name__ == "__main__":
    main()
