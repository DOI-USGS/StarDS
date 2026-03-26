#!/usr/bin/env python3
"""Setup script for STAR Python bindings."""

from setuptools import setup, find_packages
import os

# Read version
version = {}
with open("pystar/_version.py") as f:
    exec(f.read(), version)

# Read README
long_description = ""
if os.path.exists("README.md"):
    with open("README.md", "r", encoding="utf-8") as f:
        long_description = f.read()

setup(
    name="pystar",
    version=version["__version__"],
    author="USGS Astrogeology",
    author_email="astroweb@usgs.gov",
    description="STAR (Simple Tensors Arrays and Rasters) for persistent N-dimensional arrays",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/DOI-USGS/CameraStateFile",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Science/Research",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: C++",
        "Topic :: Scientific/Engineering",
        "Topic :: Software Development :: Libraries",
    ],
    python_requires=">=3.8",
    install_requires=[
        "numpy>=1.20.0",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0",
            "pytest-cov>=3.0",
        ],
    },
    package_data={
        "pystar": ["*.so", "*.pyd", "_pystar.*"],  # Include compiled extension
    },
    include_package_data=True,
    zip_safe=False,
    # Note: The C++ extension is built by CMake, not by setup.py
    # Run `make _pystar` first to build the extension
)
