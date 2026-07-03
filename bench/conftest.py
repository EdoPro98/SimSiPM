"""
Provides a no-op `benchmark` fixture when pytest-benchmark is not installed,
so bench_python.py is still runnable (each target called once, without timing).

With pytest-benchmark installed the real fixture takes precedence automatically.
"""

import pytest

try:
    import pytest_benchmark  # noqa: F401 — plugin registers the real fixture
except ImportError:
    class _NoBenchmark:
        def __call__(self, fn, *args, **kwargs):
            return fn(*args, **kwargs)

        def pedantic(self, fn, args=(), kwargs=None, setup=None,
                     rounds=1, warmup_rounds=0, iterations=1):
            if setup:
                setup()
            return fn(**(kwargs or {}))

    @pytest.fixture
    def benchmark():
        return _NoBenchmark()
