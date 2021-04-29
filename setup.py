try:
    from skbuild import setup
except ImportError:
    from setuptools import setup
from glob import glob
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11.setup_helpers import ParallelCompile
import platform
import os

ParallelCompile("NPY_NUM_BUILD_JOBS").install()

__version__ = "1.1.3-alpha"
extra_compile_args = [
    "-DNDEBUG",
    "-O3",
    "-ffast-math",
    "-funsafe-math-optimizations",
    "-mfma",
    "-mavx2",
]
extra_link_args = []

if os.environ.get("SIPM_OMP"):
    print("Using OpenMP")
    extra_compile_args.append("-fopenmp")
    extra_link_args.append("-lgomp")

if not platform.system() == "Linux":
    extra_compile_args.append("-fno-aligned-allocation")

sources = []
sources.extend(glob("src/*.cpp"))
sources.extend(glob("python/*.cpp"))
include_dirs = ["include/"]


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path
    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked."""

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11

        return pybind11.get_include(self.user)


ext_modules = [
    Pybind11Extension(
        "SiPM",
        sources=sources,
        include_dirs=[
            include_dirs,
            get_pybind_include(),
            get_pybind_include(user=True),
        ],
        define_macros=[("VERSION_INFO", __version__)],
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        language="c++",
    ),
]


setup(
    name="SiPM",
    version=__version__,
    author="Edoardo Proserpio",
    author_email="edoardo.proserpio@gmail.com",
    maintainer="Edoardo Proserpio",
    maintainer_email="edoardo.proserpio@gmail.com",
    url="https://github.com/EdoPro98/SimSiPM",
    description="Library for Silicon Photomultipliers simulation.",
    long_description="""Library for Silicon Photomultipliers simulation.
                        Developed for high energy physics and particle phisics simulations.""",
    ext_modules=ext_modules,
    include_dirs=include_dirs,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    license="MIT",
    classifiers=[
        "Development Status :: 4 - Beta",
        "License :: OSI Approved :: MIT License",
        "Topic :: Scientific/Engineering :: Physics",
        "Programming Language :: Python :: 3",
    ],
)
