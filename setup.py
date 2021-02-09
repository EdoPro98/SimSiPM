from setuptools import setup
from glob import glob

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext


__version__ = "1.0.1-alpha"
extra_compile_args = ["-DNDEBUG", "-O3"]


sources = []
sources.extend(glob("src/**/*.cpp"))
sources.extend(glob("python/*.cpp"))
include_dirs = ["SimSiPM/", "src/components/"]

ext_modules = [
    Pybind11Extension("SiPM",
                      sources=sources,
                      define_macros=[('VERSION_INFO', __version__)],
                      extra_compile_args=extra_compile_args,
                      language="c++"
                      )
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
    long_description="Library for Silicon Photomultipliers simulation. Developed for high energy physics and particle phisics simulations.",
    ext_modules=ext_modules,
    include_dirs=include_dirs,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    license="MIT",
    classifiers=["Development Status :: 4 - Beta",
                 "License :: OSI Approved :: MIT License",
                 "Topic :: Scientific/Engineering :: Physics",
                 "Programming Language :: Python :: 3"]
                 )
