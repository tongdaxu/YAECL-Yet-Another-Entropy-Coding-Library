from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.0.1"

ext_modules = [
    Pybind11Extension("yaecl",
        ["yaecl_python.cpp"],
        # Example: passing in the version to the compiled code
        define_macros = [('VERSION_INFO', __version__)],
        include_dirs = ["."]
        ),
]

setup(
    name="yaecl",
    version=__version__,
    author="Tongda Xu",
    author_email="x.tongda@nyu.edu",
    url="https://github.com/tongdaxu/YAECL-Yet-Another-Entropy-Coding-Library",
    description="YAECL: Yet Another Entropy Coding Library",
    long_description="See doc in github page: https://github.com/tongdaxu/YAECL-Yet-Another-Entropy-Coding-Library",
    ext_modules=ext_modules,
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
)
