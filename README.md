# oRatio

![Build Status](https://github.com/ratioSolver/oRatio/actions/workflows/cmake.yml/badge.svg)
[![codecov](https://codecov.io/gh/ratioSolver/oRatio/branch/master/graph/badge.svg)](https://codecov.io/gh/ratioSolver/oRatio)

> Dum loquimur, fugerit invida aetas: carpe diem, quam minimum credula postero. (Orazio, Odi, I, 11, 7-8)

oRatio is an Integrated Logic and Constraint based solver which takes inspiration from both Logic Programming (LP) and Constraint Programming (CP).

## Getting started

The oRatio system is invoked with a list of command line arguments representing the locations of the required [compilation units](https://github.com/ratioSolver/RiDDLe/wiki) (e.g. domain models, problem instances or plan adaptations) and the desired output file like in the following:

```shell
oRatio cu_0.rddl cu_1.rddl ... cu_n sol.json
```

As an example, the following code invokes the oRatio solver using the domain model defined in `examples/blocks/blocks_domain.rddl`, the problem instance specified in `examples/blocks/blocks_problem_10.rddl` and, once found a solution, writes it in the `solution.json` file:

```shell
oRatio examples/blocks/blocks_domain.rddl examples/blocks/blocks_problem_10.rddl solution.json
```

Further information about the RiDDLe language, used for specifying the input files, can be found in the corresponding [wiki](https://github.com/ratioSolver/RiDDLe/wiki) page.

## Building oRatio

The basic requirements for building oRatio are:

- [Git](https://git-scm.com/)
- [CMake](https://cmake.org) v3.x
- A C++ compiler

### Building on Linux

The easiest way to install the building requirements on Ubuntu is as follows

```shell
sudo apt-get install build-essential
sudo apt-get install cmake
```

once the building requirements are installed, move to a desired folder and clone the oRatio repository

```shell
git clone --recurse-submodules https://github.com/ratioSolver/oRatio
```

finally, build and install oRatio

```shell
mkdir build
cd build
cmake ..
sudo make install
```

### Building on OS X

The easiest way to install the building requirements on OS X consists in downloading and installing the [Command Line Tools](https://developer.apple.com/downloads/) and CMake. Once the building requirements are installed, open a terminal, move to a desired folder and clone the oRatio repository

```shell
git clone --recurse-submodules https://github.com/ratioSolver/oRatio
```

finally, build and install oRatio

```shell
mkdir build
cd build
cmake ..
sudo make install
```

### Building on Windows

The easiest way to compile oRatio on Windows is through [Visual Studio](https://www.visualstudio.com/). Download and install Visual Studio, download Git and CMake. Start a Visual Studio Command prompt, move to a desired folder and clone the oRatio repository

```shell
git clone --recurse-submodules https://github.com/ratioSolver/oRatio
```

finally, build oRatio

```shell
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake
```

### Building notes

The CMake building script, by default, generates a 'Debug' target resulting in a code that, when executed, performs several checks and is not optimized. A different target can be choosen either from the CMake gui, by setting the `CMAKE_BUILD_TYPE` parameter to the desired target, or from the command line, by providing CMake the same parameter through the `-D` option.

Available targets are:

* **Debug**, emits debug information and do not performs optimization
* **Release**, omits debug information and performs optimization
* **MinSizeRel**, optimizes for smallest binary
* **RelWithDebInfo**, emits debug information and performs optimization

As an example, an optimized target can be generated through the following command

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```
