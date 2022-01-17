import pytest
import SiPM
import numpy as np
import random
import warnings

N = 10000000  # Repeat tests 10e6 times
toll = 3  # Tollerance sigma


class TestRandom:
    def test_Rand_is_random(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            assert rng.Rand() != rng.Rand()

    def test_RandExp_is_random(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            assert rng.randExponential(1) != rng.randExponential(1)

    def test_RandNormal_is_random(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            assert rng.randGaussian(0, 1) != rng.randGaussian(0, 1)

    def test_Rand_is_in_range(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            x = rng.Rand()
            assert x > 0
            assert x < 1

    def test_RandInt_is_in_range(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            x = rng.randInteger(100)
            assert x >= 0
            assert x <= 100

    def test_RandExp_is_in_range(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            x = rng.randExponential(1)
            assert x >= 0

    def test_RandPoiss_is_in_range(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            x = rng.randPoisson(10)
            assert x >= 0

    def test_Rand_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.Rand(N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 0.5
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandIntSmall_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randInteger(10, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 4.5
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandIntBig_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randInteger(10000, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 4999.5
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandExpSmall_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randExponential(0.01, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 0.01
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandExpBig_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randExponential(100, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 100
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandNormSmall_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randGaussian(0, 0.1, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 0
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandNormBig_statistic(self):
        rng = SiPM.SiPMRandom()
        x = rng.randGaussian(0, 10, N)
        mean = np.mean(x)
        std = np.std(x) / (N ** 0.5)
        expected = 0
        Z = np.abs(mean - expected) / std
        assert Z < 3
        if Z > 1:
            warnings.warn(
                f"Unprobable mean value: {mean:.3f} instead of {expected}. This is expected for a test based on random values.",
                UserWarning,
            )

    def test_RandVector_is_random(self):
        rng = SiPM.SiPMRandom()
        rand_array = rng.Rand(N)
        for i in range(N - 1):
            assert rand_array[i] != rand_array[i + 1]

    def test_RandExpVector_is_random(self):
        rng = SiPM.SiPMRandom()
        rand_array = rng.randExponential(1, N)
        for i in range(N - 1):
            assert rand_array[i] != rand_array[i + 1]

    def test_RandNormVector_is_random(self):
        rng = SiPM.SiPMRandom()
        rand_array = rng.randGaussian(0, 1, N)
        for i in range(N - 1):
            assert rand_array[i] != rand_array[i + 1]

    def test_RandVector_is_in_range(self):
        rng = SiPM.SiPMRandom()
        rand_array = rng.Rand(N)
        for i in range(N - 1):
            assert rand_array[i] > 0
            assert rand_array[i] < 1

    def test_RandExpVector_is_in_range(self):
        rng = SiPM.SiPMRandom()
        rand_array = rng.randExponential(1, N)
        for i in range(N - 1):
            assert rand_array[i] > 0

    def test_RngSeed(self):
        rng = SiPM.SiPMRandom()
        for i in range(N):
            rng.seed(int.from_bytes(random.randbytes(8), "big"))
            x = rng.Rand()
            rng.seed(int.from_bytes(random.randbytes(8), "big"))
            y = rng.Rand()
            assert x != y

    def test_RngGeneration(self):
        rng = SiPM.SiPMRandom()
        expected = (
            2.6770408112497535e-10,
            2.2888317445790562e-05,
            0.5027885437448278,
            0.01176967815998986,
            0.3879451975033448,
            0.4092261985079124,
            0.9980280691448176,
            0.7526879616003483,
            0.570652454801827,
            0.12967310254649567,
        )
        for i in range(N):
            rng.seed(1234567890)
            for e in expected:
                assert rng.Rand() == e


class TestProperties:
    def test_SetProperty(self):
        for i in range(N):
            prop = SiPM.SiPMProperties()
            dcr = random.random() * 1e5
            xt = random.random()
            ap = random.random()

            prop.setDcr(dcr)
            prop.setXt(xt)
            prop.setAp(ap)
            assert prop.dcr() == dcr
            assert prop.xt() == xt
            assert prop.ap() == ap

    def test_SetPropertyString(self):
        for i in range(N):
            prop = SiPM.SiPMProperties()
            dcr = random.random() * 1e5
            xt = random.random()
            ap = random.random()
            dxt = random.random()

            prop.setProperty("Dcr", dcr)
            prop.setProperty("Xt", xt)
            prop.setProperty("Ap", ap)
            prop.setProperty("DXt",dxt)
            assert prop.dcr() == dcr
            assert prop.xt() == xt
            assert prop.ap() == ap
            assert prop.dxt() == dxt

    def test_SetHitDistribution(self):
        prop = SiPM.SiPMProperties()
        prop.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
        assert prop.pdeType() == SiPM.SiPMProperties.PdeType.kNoPde
        prop.setPdeType(SiPM.SiPMProperties.PdeType.kSpectrumPde)
        assert prop.pdeType() == SiPM.SiPMProperties.PdeType.kSpectrumPde
        prop.setPdeType(SiPM.SiPMProperties.PdeType.kSimplePde)
        assert prop.pdeType() == SiPM.SiPMProperties.PdeType.kSimplePde
        for p in range(1000):
            prop.setPde(p/1000)
            assert prop.pde() == p/1000
