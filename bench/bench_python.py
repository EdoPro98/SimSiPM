"""
Python-binding benchmarks for SimSiPM using pytest-benchmark.

Structure mirrors bench/random.cpp and bench/sensor.cpp so that
Python-vs-C++ overhead is straightforward to compare.

Install
-------
    pip install pytest-benchmark
    # Build and install the Python module from the repo root:
    cmake -B build -S . -DSIPM_BUILD_PYTHON=ON && make -C build -j$(nproc)
    pip install -e .

Run
---
    pytest bench/bench_python.py -v --benchmark-sort=name
    pytest bench/bench_python.py -k TestRandom --benchmark-disable-gc
    pytest bench/bench_python.py --benchmark-json=results.json
    # Compare two saved runs:
    pytest-benchmark compare before.json after.json --histogram
"""

try:
    import SiPM
except ImportError as exc:
    raise ImportError(
        "sipm module not found. Build it with:\n"
        "  cmake -B build -S . -DSIPM_BUILD_PYTHON=ON && make -C build -j$(nproc)\n"
        "  pip install -e ."
    ) from exc

import numpy as np
import pytest


# ── Shared configuration ──────────────────────────────────────────────────────

FIXED_SEED = 0xDEADBEEFCAFEBABE


def base_props():
    """1 mm sensor, 25 µm pitch (1600 cells), 500 ns window, 1 ns/sample."""
    p = SiPM.SiPMProperties()
    p.setSize(1)
    p.setPitch(25)
    p.setSignalLength(500)
    p.setSampling(1)
    return p


def no_noise_props():
    """base_props() with DCR, XT, and AP all disabled; PDE stays kNoPde (default)."""
    p = base_props()
    p.setDcrOff()
    p.setXtOff()
    p.setApOff()
    return p


def _warm_sensor(sensor, n=256):
    """Stabilise branch predictor and caches with a few dummy events."""
    for _ in range(n):
        sensor.resetState()
        sensor.addPhotons(np.array([5.0, 10.0, 15.0, 20.0]))
        sensor.runEvent()
    sensor.resetState()


# ─────────────────────────────────────────────────────────────────────────────
# 1. SiPMRandom — mirrors bench/random.cpp
# ─────────────────────────────────────────────────────────────────────────────

class TestRandom:
    """Benchmarks for the xoshiro256+ Python binding (scalar and batched)."""

    @pytest.fixture(autouse=True)
    def _rng(self):
        r = SiPM.SiPMRandom()
        r.seed(FIXED_SEED)
        for _ in range(2048):   # warm the ring buffer
            r.Rand()
        self.rng = r

    # ── Scalar ──────────────────────────────────────────────────────────────

    def test_scalar_rand(self, benchmark):
        """Uniform double [0,1): ring-buffer fetch + right-shift + fma."""
        benchmark(self.rng.Rand)

    def test_scalar_gaussian(self, benchmark):
        """Gaussian double (ACR algorithm): ~1.7 Rand() calls on fast path."""
        benchmark(self.rng.randGaussian, 0.0, 1.0)

    def test_scalar_exponential(self, benchmark):
        """Exponential double: −log(U)·µ, one log() call per sample."""
        benchmark(self.rng.randExponential, 10.0)

    def test_scalar_integer(self, benchmark):
        """Integer in [0, max): high-32-bit multiply-shift."""
        benchmark(self.rng.randInteger, 40)

    def test_scalar_poisson_small_mu(self, benchmark):
        """Poisson(µ=2): Knuth product loop, ~exp(2)≈7 Rand() calls expected."""
        benchmark(self.rng.randPoisson, 2.0)

    def test_scalar_poisson_medium_mu(self, benchmark):
        """Poisson(µ=25): Knuth product loop, ~25 Rand() calls per sample."""
        benchmark(self.rng.randPoisson, 25.0)

    def test_scalar_poisson_large_mu(self, benchmark):
        """Poisson(µ=100): Gaussian approximation (threshold >50), O(1) calls."""
        benchmark(self.rng.randPoisson, 100.0)

    # ── Batched array generation ─────────────────────────────────────────────

    @pytest.mark.parametrize("n", [64, 1_024, 16_384])
    def test_rand_double_array(self, benchmark, n):
        """Batched float64: getRand() then >>11 * 0x1p-53 in-place per element."""
        benchmark(self.rng.Rand, n)

    @pytest.mark.parametrize("n", [64, 1_024, 16_384])
    def test_gaussian_double_array(self, benchmark, n):
        """Batched Gaussian float64: Box-Muller pairs (log + sincos per 2 values)."""
        benchmark(self.rng.randGaussian, 0.0, 1.0, n)

    @pytest.mark.parametrize("n", [64, 1_024, 16_384])
    def test_gaussian_float_array(self, benchmark, n):
        """Batched Gaussian float32: single-precision Box-Muller (sqrtf + sincosf)."""
        benchmark(self.rng.randGaussianF, 0.0, 1.0, n)

    @pytest.mark.parametrize("n", [64, 1_024, 16_384])
    def test_exponential_double_array(self, benchmark, n):
        """Batched exponential float64: one log() per element."""
        benchmark(self.rng.randExponential, 10.0, n)

    @pytest.mark.parametrize("n", [64, 1_024, 16_384])
    def test_integer_array(self, benchmark, n):
        """Batched integers in [0, max): multiply-shift loop."""
        benchmark(self.rng.randInteger, 40, n)


# ─────────────────────────────────────────────────────────────────────────────
# 2. SiPMSensor — mirrors bench/sensor.cpp
# ─────────────────────────────────────────────────────────────────────────────

class TestSensorPhaseIsolation:
    """
    A. Phase isolation — each benchmark disables all effects except the one
    under study, directly mirroring the C++ SensorFixture phase-isolation group.

    addDcrEvents         → test_dcr_only
    addPhotoelectrons    → test_pe_generation_{no,simple,spectrum}_pde
    addCorrelatedNoise   → test_{crosstalk,afterpulse,xt_and_ap}_only
    """

    def test_baseline_noise_only(self, benchmark):
        """Irreducible floor: only the electronic noise baseline waveform is generated."""
        sensor = SiPM.SiPMSensor(no_noise_props())
        _warm_sensor(sensor)

        def run():
            sensor.resetState()
            sensor.runEvent()

        benchmark(run)

    @pytest.mark.parametrize("dcr_hz", [200_000, 1_000_000, 10_000_000, 100_000_000])
    def test_dcr_only(self, benchmark, dcr_hz):
        """DCR only: exponential inter-arrival sampling + random cell selection.
        Expected hits/event = rate × 500 ns:
            200 kHz → 0.1    10 MHz → 5    100 MHz → 50
        """
        p = no_noise_props()
        p.setDcr(dcr_hz)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)

        def run():
            sensor.resetState()
            sensor.runEvent()

        benchmark(run)

    @pytest.mark.parametrize("n_photons", [1, 10, 100, 1_000])
    def test_pe_generation_no_pde(self, benchmark, n_photons):
        """PE generation (kNoPde, default): every photon accepted; hitCell() + SiPMHit alloc."""
        sensor = SiPM.SiPMSensor(no_noise_props())
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100, 1_000])
    def test_pe_generation_simple_pde(self, benchmark, n_photons):
        """PE generation (kSimplePde 30%): ~70 % of photons rejected with one Rand() draw."""
        p = no_noise_props()
        p.setPde(0.30)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100, 1_000])
    def test_pe_generation_spectrum_pde(self, benchmark, n_photons):
        """PE generation (kSpectrumPde): binary-search map lookup per photon."""
        p = no_noise_props()
        p.setPdeSpectrum([300, 400, 500, 600, 700], [0.10, 0.30, 0.35, 0.20, 0.10])
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)
        wlens = rng.randGaussian(500.0, 50.0, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times, wlens)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100])
    def test_crosstalk_only(self, benchmark, n_photons):
        """XT only (10%): recursive SiPMHit allocation in adjacent cells."""
        p = no_noise_props()
        p.setXt(0.10)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100])
    def test_afterpulse_only(self, benchmark, n_photons):
        """AP only (5%): same-cell delayed hit with exponential arrival time."""
        p = no_noise_props()
        p.setAp(0.05)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100])
    def test_xt_and_ap(self, benchmark, n_photons):
        """XT + AP (realistic defaults): correlated-noise overhead vs kNoPde baseline."""
        p = no_noise_props()
        p.setXt(0.10)
        p.setAp(0.05)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)


class TestSensorAlgorithmic:
    """
    B. Algorithmic comparisons — hit distribution, geometry, signal length,
    and the full end-to-end pipeline. Mirrors the C++ algorithmic group.
    """

    @pytest.mark.parametrize("dist", [
        pytest.param(SiPM.SiPMProperties.HitDistribution.kUniform,  id="uniform"),
        pytest.param(SiPM.SiPMProperties.HitDistribution.kCircle,   id="circle"),
        pytest.param(SiPM.SiPMProperties.HitDistribution.kGaussian, id="gaussian"),
    ])
    @pytest.mark.parametrize("n_photons", [1, 64, 1_024])
    def test_hit_distribution(self, benchmark, dist, n_photons):
        """Uniform / circle / gaussian hit placement at various photon counts."""
        p = no_noise_props()
        p.setHitDistribution(dist)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("pitch_um", [
        pytest.param(100, id="100cells"),    # 10×10
        pytest.param(25,  id="1600cells"),   # 40×40
        pytest.param(10,  id="14400cells"),  # 120×120
    ])
    @pytest.mark.parametrize("n_photons", [1, 64, 1_024])
    def test_geometry(self, benchmark, pitch_um, n_photons):
        """Sensor geometry sweep: occupancy and hash-table sparsity at various cell counts."""
        p = no_noise_props()
        p.setPitch(pitch_um)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("length_ns", [100, 500, 2_000])
    @pytest.mark.parametrize("n_photons", [1, 64, 1_024])
    def test_signal_length(self, benchmark, length_ns, n_photons):
        """Signal length sweep: waveform generation is O(n_hits × n_samples)."""
        p = no_noise_props()
        p.setSignalLength(length_ns)
        sensor = SiPM.SiPMSensor(p)
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100, 1_000])
    def test_full_simulation(self, benchmark, n_photons):
        """Full simulation (DCR + XT + AP + SimplePDE): primary regression target."""
        sensor = SiPM.SiPMSensor(base_props())
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def setup():
            sensor.resetState()
            sensor.addPhotons(times)

        benchmark.pedantic(sensor.runEvent, setup=setup, rounds=200, warmup_rounds=10)

    @pytest.mark.parametrize("n_photons", [1, 10, 100, 1_000])
    def test_full_event_with_analysis(self, benchmark, n_photons):
        """Complete pipeline: simulate → extract integral / peak / tot."""
        sensor = SiPM.SiPMSensor(base_props())
        _warm_sensor(sensor)
        rng = SiPM.SiPMRandom()
        rng.seed(FIXED_SEED)
        times = rng.randGaussian(10.0, 0.1, n_photons)

        def run():
            sensor.resetState()
            sensor.addPhotons(times)
            sensor.runEvent()
            sig = sensor.signal()
            sig.integral(5.0, 250.0, 0.5)
            sig.peak(5.0, 250.0, 0.5)
            sig.tot(5.0, 250.0, 0.5)

        benchmark(run)


# ─────────────────────────────────────────────────────────────────────────────
# 3. SiPMAnalogSignal — mirrors bench/sensor.cpp section C
# ─────────────────────────────────────────────────────────────────────────────

@pytest.fixture(scope="module")
def pre_generated_signal():
    """Fixed 100-PE signal on 500 ns / 1 ns window — read-only, shared."""
    rng = SiPM.SiPMRandom()
    rng.seed(0xABCD1234)
    p = SiPM.SiPMProperties()
    p.setSignalLength(500)
    p.setSampling(1)
    sensor = SiPM.SiPMSensor(p)
    times = rng.randGaussian(10.0, 0.5, 100)
    sensor.addPhotons(times)
    sensor.runEvent()
    return sensor.signal()


class TestSignalFeatures:
    """
    C. Signal feature extraction — fixed pre-generated waveform so only the
    analysis cost is measured, not simulation cost.
    Mirrors the SignalFixture group in bench/sensor.cpp.

    The waveform stays hot in L1/L2 cache throughout; the reported ns/op
    reflects the scan cost over that resident buffer.
    """

    def test_integral(self, benchmark, pre_generated_signal):
        """Integral: full float array scan gated by threshold."""
        benchmark(pre_generated_signal.integral, 5.0, 450.0, 0.5)

    def test_peak(self, benchmark, pre_generated_signal):
        """Peak: linear scan for the maximum sample above threshold."""
        benchmark(pre_generated_signal.peak, 5.0, 450.0, 0.5)

    def test_time_over_threshold(self, benchmark, pre_generated_signal):
        """ToT: count samples above threshold × peak."""
        benchmark(pre_generated_signal.tot, 5.0, 450.0, 0.5)

    def test_time_of_arrival(self, benchmark, pre_generated_signal):
        """ToA: first sample crossing threshold × peak."""
        benchmark(pre_generated_signal.toa, 5.0, 450.0, 0.5)

    def test_time_of_peak(self, benchmark, pre_generated_signal):
        """ToP: sample index of the maximum."""
        benchmark(pre_generated_signal.top, 5.0, 450.0, 0.5)

    def test_all_features(self, benchmark, pre_generated_signal):
        """All five features extracted sequentially: measures cache-reuse benefit."""
        sig = pre_generated_signal

        def extract_all():
            sig.integral(5.0, 450.0, 0.5)
            sig.peak(5.0, 450.0, 0.5)
            sig.tot(5.0, 450.0, 0.5)
            sig.toa(5.0, 450.0, 0.5)
            sig.top(5.0, 450.0, 0.5)

        benchmark(extract_all)

    @pytest.mark.parametrize("length_ns", [64, 256, 1_024, 4_096, 16_384])
    def test_integral_vs_length(self, benchmark, length_ns):
        """Integral scan cost as a function of waveform length: should be O(N)."""
        rng = SiPM.SiPMRandom()
        rng.seed(42)
        p = SiPM.SiPMProperties()
        p.setSignalLength(length_ns)
        p.setSampling(1)
        sensor = SiPM.SiPMSensor(p)
        times = rng.randGaussian(10.0, 0.5, 100)
        sensor.addPhotons(times)
        sensor.runEvent()
        sig = sensor.signal()

        benchmark(sig.integral, 5.0, float(length_ns) - 10.0, 0.5)
