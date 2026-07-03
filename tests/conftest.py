"""Pytest configuration and shared fixtures for SimSiPM Python binding tests."""
import sys
import os

import pytest

# Add the build directory to sys.path so `import SiPM` works.
_build = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "build"))
if os.path.isdir(_build) and _build not in sys.path:
    sys.path.insert(0, _build)

try:
    import SiPM
except ModuleNotFoundError:
    pytest.exit(
        "SiPM module not found. Build the Python bindings first:\n"
        "  cmake -B build -S . -DSIPM_BUILD_PYTHON=ON && make -C build SiPM",
        returncode=1,
    )

import numpy as np  # noqa: E402


# ── Shared fixtures ────────────────────────────────────────────────────────────

@pytest.fixture
def props():
    """Default SiPMProperties."""
    return SiPM.SiPMProperties()


@pytest.fixture
def clean_props():
    """SiPMProperties with all noise disabled and kNoPde."""
    p = SiPM.SiPMProperties()
    p.setDcrOff()
    p.setXtOff()
    p.setApOff()
    p.setPdeType(SiPM.SiPMProperties.PdeType.kNoPde)
    p.setSnr(100)
    p.setCcgv(0)
    return p


@pytest.fixture
def sensor(clean_props):
    """Clean SiPMSensor (no noise, NoPde) ready for deterministic tests."""
    return SiPM.SiPMSensor(clean_props)


@pytest.fixture
def rng():
    """Seeded SiPMRandom for reproducible statistical tests."""
    r = SiPM.SiPMRandom()
    r.seed(12345)
    return r
