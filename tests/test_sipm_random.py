"""Tests for SiPMRandom Python bindings."""
import math

import numpy as np
import pytest
import SiPM

N = 100_000    # sample count for mean/variance assertions
N_SMALL = 10_000


# ── Construction and seeding ──────────────────────────────────────────────────

class TestConstruction:

    def test_default_constructor(self):
        assert SiPM.SiPMRandom() is not None

    def test_seed_does_not_raise(self):
        SiPM.SiPMRandom().seed(42)


class TestSeedDeterminism:

    @pytest.mark.parametrize("seed", [1, 42, 12345, 0xDEADBEEF])
    def test_same_seed_same_rand(self, seed):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(seed); r2.seed(seed)
        for _ in range(100):
            assert r1.Rand() == r2.Rand()

    def test_same_seed_same_gaussian(self):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(99); r2.seed(99)
        for _ in range(100):
            assert r1.randGaussian(0, 1) == r2.randGaussian(0, 1)

    def test_same_seed_same_integer(self):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(77); r2.seed(77)
        for _ in range(100):
            assert r1.randInteger(100) == r2.randInteger(100)

    def test_same_seed_same_exponential(self):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(55); r2.seed(55)
        for _ in range(100):
            assert r1.randExponential(5.0) == r2.randExponential(5.0)

    def test_different_seeds_diverge(self):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(1); r2.seed(2)
        assert [r1.Rand() for _ in range(10)] != [r2.Rand() for _ in range(10)]


# ── Rand() scalar: uniform (0, 1] ─────────────────────────────────────────────

class TestRandScalar:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_returns_float(self):
        assert isinstance(self.r.Rand(), float)

    def test_in_unit_interval(self):
        for _ in range(10_000):
            x = self.r.Rand()
            assert 0.0 < x <= 1.0

    def test_mean(self):
        vals = [self.r.Rand() for _ in range(N)]
        assert abs(sum(vals) / N - 0.5) < 5.0 / math.sqrt(12.0 * N)

    def test_variance(self):
        arr = np.array([self.r.Rand() for _ in range(N_SMALL)])
        assert arr.var() == pytest.approx(1.0 / 12.0, rel=0.05)

    def test_chi2_uniformity(self):
        K = 50
        counts = np.zeros(K, dtype=int)
        for _ in range(N):
            b = int((1.0 - self.r.Rand()) * K)  # binning needs [0,1)
            counts[b] += 1
        expected = N / K
        chi2 = sum((c - expected) ** 2 / expected for c in counts)
        # chi2(K-1): mean = K-1, std = sqrt(2*(K-1)); allow 5-sigma
        assert abs(chi2 - (K - 1)) < 5.0 * math.sqrt(2.0 * (K - 1))


# ── randInteger() scalar ───────────────────────────────────────────────────────

class TestRandIntegerScalar:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_returns_int(self):
        assert isinstance(self.r.randInteger(100), int)

    def test_in_range(self):
        for _ in range(10_000):
            assert 0 <= self.r.randInteger(100) < 100

    def test_mean(self):
        vals = [self.r.randInteger(10) for _ in range(N)]
        tol = 5.0 * 10 / math.sqrt(12.0 * N)
        assert abs(sum(vals) / N - 4.5) < tol


# ── randGaussian() scalar ──────────────────────────────────────────────────────

class TestRandGaussianScalar:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_returns_float(self):
        assert isinstance(self.r.randGaussian(0.0, 1.0), float)

    def test_mean_zero(self):
        vals = [self.r.randGaussian(0.0, 1.0) for _ in range(N)]
        assert abs(sum(vals) / N) < 5.0 / math.sqrt(N)

    @pytest.mark.parametrize("mu,sigma", [(5.0, 2.0), (-3.0, 0.5), (100.0, 10.0)])
    def test_mean_and_std(self, mu, sigma):
        arr = np.array([self.r.randGaussian(mu, sigma) for _ in range(N_SMALL)])
        assert arr.mean() == pytest.approx(mu, abs=5.0 * sigma / math.sqrt(N_SMALL))
        assert arr.std() == pytest.approx(sigma, rel=0.05)


# ── randExponential() scalar ───────────────────────────────────────────────────

class TestRandExponentialScalar:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_returns_float(self):
        assert isinstance(self.r.randExponential(1.0), float)

    def test_always_positive(self):
        for _ in range(10_000):
            assert self.r.randExponential(5.0) >= 0.0

    @pytest.mark.parametrize("mu", [0.001, 1.0, 5.0, 100.0])
    def test_mean(self, mu):
        arr = np.array([self.r.randExponential(mu) for _ in range(N_SMALL)])
        assert arr.mean() == pytest.approx(mu, rel=0.05)

    def test_variance(self):
        mu = 5.0
        arr = np.array([self.r.randExponential(mu) for _ in range(N_SMALL)])
        assert arr.var() == pytest.approx(mu ** 2, rel=0.1)


# ── randPoisson() ──────────────────────────────────────────────────────────────

class TestRandPoisson:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_returns_int(self):
        assert isinstance(self.r.randPoisson(5.0), int)

    def test_always_non_negative(self):
        for _ in range(10_000):
            assert self.r.randPoisson(1.0) >= 0

    def test_zero_mu_returns_zero(self):
        for _ in range(100):
            assert self.r.randPoisson(0.0) == 0

    @pytest.mark.parametrize("mu", [0.5, 1.0, 5.0, 50.0])
    def test_mean(self, mu):
        vals = np.array([self.r.randPoisson(mu) for _ in range(N)], dtype=float)
        tol = 5.0 * math.sqrt(mu) / math.sqrt(N)
        assert abs(vals.mean() - mu) < tol

    def test_variance_equals_mean(self):
        mu = 5.0
        vals = np.array([self.r.randPoisson(mu) for _ in range(N_SMALL)], dtype=float)
        assert vals.var() == pytest.approx(mu, rel=0.1)


# ── Array-returning methods: type, dtype, shape ────────────────────────────────

class TestArrayReturnType:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    @pytest.mark.parametrize("method,args,expected_dtype", [
        ("Rand",             (100,),          np.float64),
        ("randGaussian",     (0.0, 1.0, 100), np.float64),
        ("randGaussianF",    (0.0, 1.0, 100), np.float32),
        ("randInteger",      (100, 100),       np.uint32),
        ("randExponential",  (1.0, 100),       np.float64),
    ])
    def test_returns_ndarray(self, method, args, expected_dtype):
        result = getattr(self.r, method)(*args)
        assert isinstance(result, np.ndarray)
        assert result.dtype == expected_dtype

    @pytest.mark.parametrize("method,args", [
        ("Rand",             (500,)),
        ("randGaussian",     (0.0, 1.0, 500)),
        ("randGaussianF",    (0.0, 1.0, 500)),
        ("randInteger",      (100, 500)),
        ("randExponential",  (1.0, 500)),
    ])
    def test_shape(self, method, args):
        result = getattr(self.r, method)(*args)
        assert result.shape == (500,)

    def test_single_element_shape(self):
        assert self.r.Rand(1).shape == (1,)

    def test_large_array_shape(self):
        assert self.r.Rand(1_000_000).shape == (1_000_000,)


# ── Array values and statistics ────────────────────────────────────────────────

class TestArrayValues:

    def setup_method(self):
        self.r = SiPM.SiPMRandom(); self.r.seed(42)

    def test_rand_in_unit_interval(self):
        arr = self.r.Rand(N_SMALL)
        assert np.all(arr >= 0.0) and np.all(arr <= 1.0)

    def test_rand_mean(self):
        arr = self.r.Rand(N)
        assert abs(arr.mean() - 0.5) < 5.0 / math.sqrt(12.0 * N)

    def test_rand_gaussian_mean_and_std(self):
        arr = self.r.randGaussian(0.0, 2.0, N)
        assert abs(arr.mean()) < 5.0 * 2.0 / math.sqrt(N)
        assert arr.std() == pytest.approx(2.0, rel=0.02)

    def test_rand_gaussianF_mean(self):
        arr = self.r.randGaussianF(0.0, 1.0, N).astype(np.float64)
        assert abs(arr.mean()) < 5.0 / math.sqrt(N)

    def test_rand_integer_range(self):
        arr = self.r.randInteger(100, N_SMALL)
        assert np.all(arr < 100) and np.all(arr >= 0)

    def test_rand_integer_mean(self):
        arr = self.r.randInteger(10, N).astype(float)
        tol = 5.0 * 10 / math.sqrt(12.0 * N)
        assert abs(arr.mean() - 4.5) < tol

    def test_rand_exponential_positive(self):
        assert np.all(self.r.randExponential(5.0, N_SMALL) >= 0.0)

    def test_rand_exponential_mean(self):
        mu = 5.0
        arr = self.r.randExponential(mu, N_SMALL)
        assert abs(arr.mean() - mu) < 5.0 * mu / math.sqrt(N_SMALL)


# ── Seed determinism for array methods ────────────────────────────────────────

class TestArraySeedDeterminism:

    @pytest.mark.parametrize("seed,method,args", [
        (111, "Rand",            (1000,)),
        (222, "randGaussian",    (0.0, 1.0, 1000)),
        (333, "randGaussianF",   (0.0, 1.0, 1000)),
        (444, "randInteger",     (100, 1000)),
        (555, "randExponential", (5.0, 1000)),
    ])
    def test_deterministic(self, seed, method, args):
        r1, r2 = SiPM.SiPMRandom(), SiPM.SiPMRandom()
        r1.seed(seed); r2.seed(seed)
        np.testing.assert_array_equal(
            getattr(r1, method)(*args),
            getattr(r2, method)(*args),
        )
