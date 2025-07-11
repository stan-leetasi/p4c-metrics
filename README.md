<!--!
\page getting_started Getting Started                                    
-->
<!--!
\internal
-->
# P4C
<!--!
\endinternal
-->
<!--!
[TOC]
-->
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-debian.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-debian.yml)
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-fedora.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-fedora.yml)
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-mac.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-mac.yml)
[![Bazel Build](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml)
[![Validation](https://github.com/p4lang/p4c/actions/workflows/ci-validation-nightly.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-validation-nightly.yml)
[![Docker Container](https://github.com/p4lang/p4c/actions/workflows/ci-container-image.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-container-image.yml)

<!--!
\internal
-->
* [Sample Backends in P4C](#sample-backends-in-p4c)
* [Getting started](#getting-started)
   * [Installing packaged versions of P4C](#installing-packaged-versions-of-p4c)
   * [Installing P4C from source](#installing-p4c-from-source)
* [Dependencies](#dependencies)
   * [Ubuntu dependencies](#ubuntu-dependencies)
   * [Fedora dependencies](#fedora-dependencies)
   * [macOS dependencies](#macos-dependencies)
   * [Garbage collector](#garbage-collector)
   * [Crash dumps](#crash-dumps)
* [Development tools](#development-tools)
   * [Git setup](#git-setup)
* [Docker](#docker)
* [Bazel](#bazel)
* [Build system](#build-system)
   * [Defining new CMake targets](#defining-new-cmake-targets)
* [Known issues](#known-issues)
   * [Frontend](#frontend)
   * [Backends](#backends)
* [How to Contribute](#how-to-contribute)
* [P4 Compiler Onboarding](#p4-compiler-onboarding)
* [Contact](#contact)
<!--!
\endinternal
-->
P4C is a reference compiler for the P4 programming language.
It supports both P4-14 and P4-16; you can find more information about P4
[here](http://p4.org) and the specifications for both versions of the language
[here](https://p4.org/specs).
One fact attesting to the level of quality and completeness of P4C's
code is that its front-end code, mid-end code, and P4C-graphs back end
are used as the basis for at least one commercially supported P4
compiler.

P4C is modular; it provides a standard frontend and midend which can be combined
with a target-specific backend to create a complete P4 compiler. The goal is to
make adding new backends easy.

<!--!
\include{doc} "../docs/doxygen/01_overview.md"
-->
## Sample Backends in P4C
P4C includes seven sample backends, catering to different target architectures and use cases:
* p4c-bm2-ss: can be used to target the P4 `simple_switch` written using
  the [BMv2 behavioral model](https://github.com/p4lang/behavioral-model),
* p4c-dpdk: can be used to target the [DPDK software switch (SWX) pipeline](https://doc.dpdk.org/guides/rel_notes/release_20_11.html),
* p4c-ebpf: can be used to generate C code which can be compiled to [eBPF](https://en.wikipedia.org/wiki/Berkeley_Packet_Filter)
  and then loaded in the Linux kernel. The eBPF backend currently implements three architecture models:
  [ebpf_model.p4 for packet filtering, xdp_model.p4 for XDP](./backends/ebpf/README.md) and
  [the fully-featured PSA (Portable Switch Architecture) model](./backends/ebpf/psa/README.md).
* p4test: a source-to-source P4 translator which can be used for
  testing, learning compiler internals and debugging,
* p4c-graphs: can be used to generate visual representations of a P4 program;
  for now it only supports generating graphs of top-level control flows, and
* p4c-ubpf: can be used to generate eBPF code that runs in user-space.
* p4tools: a platform for P4 test utilities, including a test-case generator for P4 programs.
Sample command lines:

Compile P4_16 or P4_14 source code.  If your program successfully
compiles, the command will create files with the same base name as the
P4 program you supplied, and the following suffixes instead of the
`.p4`:

+ a file with suffix `.p4i`, which is the output from running the
  preprocessor on your P4 program.
+ a file with suffix `.json` that is the JSON file format expected by
  BMv2 behavioral model `simple_switch`.

```bash
p4c --target bmv2 --arch v1model my-p4-16-prog.p4
p4c --target bmv2 --arch v1model --std p4-14 my-p4-14-prog.p4
```

By adding the option `--p4runtime-files <filename>.txt` as shown in
the example commands below, P4C will also create a file
`<filename>.txt`.  This is a text format "P4Info" file, containing a
description of the tables and other objects in your P4 program that
have an auto-generated control plane API.

```
p4c --target bmv2 --arch v1model --p4runtime-files my-p4-16-prog.p4info.txt my-p4-16-prog.p4
p4c --target bmv2 --arch v1model --p4runtime-files my-p4-14-prog.p4info.txt --std p4-14 my-p4-14-prog.p4
```

All of these commands take the `--help` argument to show documentation
of supported command line options.  `p4c --target-help` shows the
supported "target, arch" pairs.

```bash
p4c --help
p4c --target-help
```

Auto-translate P4_14 source to P4_16 source:

```bash
p4test --std p4-14 my-p4-14-prog.p4 --pp auto-translated-p4-16-prog.p4
```

Check syntax of P4_16 or P4_14 source code, without limitations that
might be imposed by any particular compiler back end.  There is no
output for these commands other than error and/or warning messages.

```bash
p4test my-p4-16-prog.p4
p4test --std p4-14 my-p4-14-prog.p4
```

Generate GraphViz ".dot" files for parsers and controls of a P4_16 or
P4_14 source program.

```bash
p4c-graphs my-p4-16-prog.p4
p4c-graphs --std p4-14 my-p4-14-prog.p4
```

Generate PDF of parser instance named "ParserImpl" generated by the
`p4c-graphs` command above (search for graphviz below for its install
instructions):

```bash
dot -Tpdf ParserImpl.dot > ParserImpl.pdf
```

## Getting started

### Installing packaged versions of P4C

P4C has package support for several Ubuntu and Debian distributions.

#### Ubuntu

A P4C package is available in the following repositories for Ubuntu 20.04 and newer.

```bash
source /etc/lsb-release
echo "deb http://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/ /" | sudo tee /etc/apt/sources.list.d/home:p4lang.list
curl -fsSL https://download.opensuse.org/repositories/home:p4lang/xUbuntu_${DISTRIB_RELEASE}/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
sudo apt-get update
sudo apt install p4lang-p4c
```

#### Debian

For Debian 11 (Bullseye) it can be installed as follows:

```bash
echo 'deb https://download.opensuse.org/repositories/home:/p4lang/Debian_11/ /' | sudo tee /etc/apt/sources.list.d/home:p4lang.list
curl -fsSL https://download.opensuse.org/repositories/home:p4lang/Debian_11/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
sudo apt update
sudo apt install p4lang-p4c
```

If you cannot use a repository to install P4C, you can download the `.deb` file
for your release and install it manually. You need to download a new file each
time you want to upgrade P4C.

1. Go to [p4lang-p4c package page on OpenSUSE Build Service](https://build.opensuse.org/package/show/home:p4lang/p4lang-p4c), click on
"Download package" and choose your operating system version.

2. Install P4C, changing the path below to the path where you downloaded the package.

```bash
sudo dpkg -i /path/to/package.deb
```

### Installing P4C from source
1.  Clone the repository. It includes submodules, so be sure to use
    `--recursive` to pull them in:
    ```
    git clone --recursive https://github.com/p4lang/p4c.git
    ```
    If you forgot `--recursive`, you can update the submodules at any time using:
    ```
    git submodule update --init --recursive
    ```

2.  Install [dependencies](#dependencies). You can find specific instructions
    for Ubuntu 22.04 [here](#ubuntu-dependencies) and for macOS 11
    [here](#macos-dependencies).  You can also look at the
    [CI installation script](https://github.com/p4lang/p4c/blob/main/tools/ci-build.sh).

3.  Build. Building should also take place in a subdirectory named `build`.
    ```
    mkdir build
    cd build
    cmake .. <optional arguments>
    make -j4
    make -j4 check
    ```
    The cmake command takes the following optional arguments to
    further customize the build (see file `CMakeLists.txt` for the full list):
     - `-DCMAKE_BUILD_TYPE=Release|Debug` -- set CMAKE_BUILD_TYPE to
      Release or Debug to build with optimizations or with debug
      symbols to run in gdb. Default is Release.
     - `-DCMAKE_INSTALL_PREFIX=<path>` -- set the directory where
       `make install` installs the compiler. Defaults to /usr/local.
     - `-DENABLE_BMV2=ON|OFF`. Enable [the bmv2 backend](backends/bmv2/README.md). Default ON.
     - `-DENABLE_EBPF=ON|OFF`. Enable [the ebpf backend](backends/ebpf/README.md). Default ON.
     - `-DENABLE_P4TC=ON|OFF`. Enable [the TC backend](backends/tc/README.md). Default ON.
     - `-DENABLE_UBPF=ON|OFF`. Enable [the ubpf backend](backends/ubpf/README.md). Default ON.
     - `-DENABLE_DPDK=ON|OFF`. Enable [the DPDK backend](backends/dpdk/README.md). Default ON.
     - `-DENABLE_P4C_GRAPHS=ON|OFF`. Enable [the p4c-graphs backend](backends/graphs/README.md). Default ON.
     - `-DENABLE_P4FMT=ON|OFF`. Enable [the p4fmt backend](backends/p4fmt/README.md). Default ON.
     - `-DENABLE_P4TEST=ON|OFF`. Enable [the p4test backend](backends/p4test/README.md). Default ON.
     - `-DENABLE_TEST_TOOLS=ON|OFF`. Enable [the p4tools backend](backends/p4tools/README.md). Default OFF.
     - `-DENABLE_DOCS=ON|OFF`. Build documentation. Default is OFF.
     - `-DENABLE_GC=ON|OFF`. Enable the use of the garbage collection
       library. Default is ON.
     - `-DENABLE_GTESTS=ON|OFF`. Enable building and running GTest unit tests.
       Default is ON.
     - `-DP4C_USE_PREINSTALLED_ABSEIL=ON|OFF`. Try to find a system version of Abseil instead of a fetched one. Default is OFF.
     - `-DP4C_USE_PREINSTALLED_PROTOBUF=ON|OFF`. Try to find a system version of Protobuf instead of a CMake version. Default is OFF.
     - `-DENABLE_ABSEIL_STATIC=ON|OFF`. Enable the use of static abseil libraries. Default is ON. Only has an effect when `P4C_USE_PREINSTALLED_ABSEIL` is enabled.
     - `-DENABLE_PROTOBUF_STATIC=ON|OFF`. Enable the use of static protobuf libraries. Default is ON.
       Only has an effect when `P4C_USE_PREINSTALLED_PROTOBUF` is enabled.
     - `-DENABLE_MULTITHREAD=ON|OFF`. Use multithreading.  Default is
       OFF.
     - `-DBUILD_LINK_WITH_GOLD=ON|OFF`. Use Gold linker for build if available.
     - `-DBUILD_LINK_WITH_LLD=ON|OFF`. Use LLD linker for build if available (overrides `BUILD_LINK_WITH_GOLD`).
     - `-DENABLE_LTO=ON|OFF`. Use Link Time Optimization (LTO).  Default is OFF.
     - `-DENABLE_WERROR=ON|OFF`. Treat warnings as errors.  Default is OFF.
     - `-DCMAKE_UNITY_BUILD=ON|OFF `. Enable [unity builds](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html) for faster compilation.  Default is OFF.

    If adding new targets to this build system, please see
    [instructions](#defining-new-cmake-targets).

4.  (Optional) Install the compiler and the P4 shared headers globally.
    ```
    sudo make install
    ```
    The compiler driver `p4c` and binaries for each of the backends are
    installed in `/usr/local/bin` by default; the P4 headers are placed in
    `/usr/local/share/p4c`.

5.  You're ready to go! You should be able to compile a P4-16 program for BMV2
    using:
    ```
    p4c -b bmv2-ss-p4org program.p4 -o program.bmv2.json
    ```

If you plan to contribute to P4C, you'll find more useful information
[here](#development-tools).

## Dependencies

Ubuntu 22.04 is the officially supported platform for P4C. There's also
unofficial support for macOS 11. Other platforms are untested; you can try to
use them, but YMMV.

- A C++17 compiler. GCC 9.1 or later or Clang 6.0 or later is required.

- `git` for version control

- CMake 3.16.3 or higher

- Boehm-Weiser garbage-collector C++ library

- GNU Bison and Flex for the parser and lexical analyzer generators.

- Google Protocol Buffers v3.25.3 or higher for control plane API generation

- C++ boost library

- Python 3 and uv for scripting and running tests

- Optional: Documentation generation requires Doxygen (1.13.2) and Graphviz (2.38.0 or higher).

Backends may have additional dependencies. The dependencies for the backends
included with `P4C` are documented here:
  * [BMv2](backends/bmv2/README.md)
  * [eBPF](backends/ebpf/README.md)
  * [graphs](backends/graphs/README.md)

### Ubuntu dependencies

Most dependencies can be installed using `apt-get install`:

```bash
sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex \
libfl-dev libboost-dev libboost-iostreams-dev \
libboost-graph-dev llvm pkg-config python3 python3-pip \
tcpdump

```
Python dependencies can be installed using `uv`:
```bash
curl -LsSf https://astral.sh/uv/0.6.12/install.sh | sh
uv sync
```

**For documentation building:**

**Tools**
- Download the Doxygen 1.13.2 binary 
```bash
wget https://github.com/doxygen/doxygen/releases/download/Release_1_13_0/doxygen-1.13.2.linux.bin.tar.gz
```
- Extract and install Doxygen 
```bash
tar xzvf doxygen-1.13.2.linux.bin.tar.gz
cd doxygen-1.13.2
sudo make install
cd .. 
```
- Install Graphviz
```bash
sudo apt-get install -y graphviz
```
**Theme** 
```bash
git clone --depth 1 -b v2.3.4 https://github.com/jothepro/doxygen-awesome-css ./docs/doxygen/awesome_css
```

`P4C` also depends on Google Protocol Buffers (Protobuf). `P4C` requires version
3.0 or higher, so the packaged version provided in Ubuntu 22.04 **should**
work. However, P4C typically installs its own version of Protobuf using CMake's `FetchContent` module
(at the moment, 3.25.3). If you are experiencing issues with the Protobuf version shipped with your OS distribution, we recommend that to install Protobuf 3.25.3 from source. You can find instructions
[here](https://github.com/protocolbuffers/protobuf/blob/v3.25.3/src/README.md).
After cloning Protobuf and before you build, check-out version 3.25.3:

`git checkout v3.25.3`

Please note that while all Protobuf versions newer than 3.0 should work for
P4C itself, you may run into trouble with Abseil, some extensions and other p4lang
projects unless you install version 3.25.3.

P4C also depends on Google Abseil library. This library is also a pre-requisite for Protobuf of any version newer than 3.21. Therefore the use of Protobuf of suitable version automatically fulfils Abseil dependency. P4C typically installs its own version of Abseil using CMake's `FetchContent` module (Abseil LTS 20240116.1 at the moment).

#### CMake
P4C requires a CMake version of at least 3.16.3 or higher. On older systems, a newer version of CMake can be installed using `pip3 install --user cmake==3.16.3`. We have a CI test on Ubuntu 18.04 that uses this option, but there is no guarantee that this will lead to a successful build.

### Fedora dependencies

```bash
sudo dnf install -y cmake g++ git automake libtool gc-devel bison flex \
libfl-devel gmp-devel boost-devel boost-iostreams boost-graph llvm pkg-config \
python3 python3-pip tcpdump uv

uv sync
```

**For documentation building:**

**Tools**
- Download the Doxygen 1.13.2 binary
```bash 
wget https://github.com/doxygen/doxygen/releases/download/Release_1_13_0/doxygen-1.13.2.linux.bin.tar.gz
```
- Extract and install Doxygen
```bash
tar xzvf doxygen-1.13.2.linux.bin.tar.gz
cd doxygen-1.13.2
sudo make install
cd ..
```
- Install Graphviz
```bash
sudo dnf install -y graphviz
```
**Theme**
```bash
git clone --depth 1 -b v2.3.4 https://github.com/jothepro/doxygen-awesome-css ./docs/doxygen/awesome_css
```

You can also look at the [dependencies installation script](https://github.com/p4lang/p4c/blob/main/tools/install_fedora_deps.sh)
for a fresh Fedora instance.

### macOS dependencies

Installing on macOS:

- Enable XCode's command-line tools:
  ```
  xcode-select --install
  ```

- Install Homebrew:
  ```
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  ```
  Be sure to add `/usr/local/bin/` to your `$PATH`.

- Install dependencies using Homebrew:
  ```
  brew install autoconf automake libtool bdw-gc boost bison pkg-config
  ```
  or with MacPorts
  ```
  sudo port install autoconf automake coreutils libtool boehmgc boost bison pkg-config
  ```

  By default, Homebrew doesn't link programs into `/usr/local/bin` if
  they would conflict with a version provided by the base system. This
  includes Bison, since an older version ships with macOS. `make
  check` depends on the newer Bison we just installed from Homebrew
  (see [#83](http://github.com/p4lang/p4c/issues/83)), so you'll want
  to add it to your `$PATH` one way or another. One simple way to do
  that is to request that Homebrew link it into `/usr/local/bin`:
  ```
  brew link --force bison
  ```

  **Optional documentation building tools:**
  -  Download and install the Doxygen 1.13.2 DMG file from [here](https://github.com/doxygen/doxygen/releases/download/Release_1_13_0/Doxygen-1.13.2.dmg).
  - Install Graphviz
  ```
  brew install graphviz
  ```
  **Optional Documentation theme:** 
  ```
  git clone --depth 1 -b v2.3.4 https://github.com/jothepro/doxygen-awesome-css ./docs/doxygen/awesome_css
  ```

  Homebrew offers a `protobuf` formula. It installs version 3.2, which should
  work for P4C itself but may cause problems with some extensions. It's
  preferable to use the version of Protobuf which is supplied with CMake's fetchcontent (3.25.3).

  The `protobuf` formula requires the following CMake variables to be set,
  otherwise CMake does not find the libraries or fails in linking. It is likely
  that manually installed Protobuf will require similar treatment.

  ```
  PB_PREFIX="$(brew --prefix --installed protobuf)"
  ./bootstrap.sh \
    -DProtobuf_INCLUDE_DIR="${PB_PREFIX}/include/" \
    -DProtobuf_LIBRARY="${PB_PREFIX}/lib/libprotobuf.dylib" \
    -DENABLE_PROTOBUF_STATIC=OFF
  ```

### Garbage collector

P4c relies on [BDW garbage collector](https://github.com/ivmai/bdwgc)
to manage its memory.  By default, the P4C executables are linked with
the garbage collector library.  When the GC causes problems, this can
be disabled by setting `ENABLE_GC` cmake option to `OFF`.  However,
this will dramatically increase the memory usage by the compiler, and
may become impractical for compiling large programs.  **Do not disable
the GC**, unless you really have to.  We have noticed that this may be
a problem on MacOS.

### Crash dumps

P4c will use [libbacktrace](https://github.com/ianlancetaylor/libbacktrace.git)
to produce readable crash dumps if it is available.  This is an optional
dependency; if it is not available everything should build just fine, but
crash dumps will not be very readable.

## Development tools

There is a variety of design and development documentation [here](docs/README.md).

We recommend using `clang++` with no optimizations for speeding up
compilation and simplifying debugging.

We recommend installing a new version of [gdb](http://ftp.gnu.org/gnu/gdb),
because older gdb versions do not always handle C++11 or newer correctly.

We recommend exuberant ctags for navigating source code in Emacs and vi.  `sudo
apt-get install exuberant-ctags.` The Makefile targets `make ctags` and `make
etags` generate tags for vi and Emacs respectively.  (Make sure that you are
using the correct version of ctags; there are several competing programs with
the same name in existence.)

To build code documentation, after installing Doxygen and the other
required packages:

```bash
# Starting from root directory of your copy of p4c repo
cd docs/doxygen
doxygen doxygen.cfg
```
The HTML output is available in
`docs/doxygen/build/html/index.html`.

### Git setup

Occasionally formatting commits are applied to P4C. These pollute the git history. To ignore these commits in git blame, run this command
```git config blame.ignoreRevsFile .git-blame-ignore-revs```

The P4C code base is subject to a series of linter checks which are checked by CI. To avoid failing these checks and wasting unnecessary CI cycles and resources, you can install [git commit hooks](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks) by running
```./tools/install_git_hooks.sh```
These commit hooks will run on every commit and check the files you are planning to commit with cpplint and clang-format.

## Docker

A Dockerfile is included. You can generate an image which contains a copy of P4C
in `/p4c/build` by running:

```
docker build -t p4c .
```

On some platforms Docker limits the memory usage of any container, even
containers used during the `docker build` process. On macOS in particular the
default is 2GB, which is not enough to build P4C. Increase the memory limit to
at least 4GB via Docker preferences or you are likely to see "internal compiler
errors" from GCC which are caused by low memory.

## Bazel
[![Bazel Build](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml)

The project can also be build using [Bazel](https://bazel.build):
```sh
bazel build //...
```
We run continuous integration to ensure this works with the latest version of
Bazel.

We also provide a [`p4_library` rule](https://github.com/p4lang/p4c/blob/main/bazel/p4_library.bzl) for invoking
P4C during the build process of 3rd party Bazel projects.

See [bazel/example](https://github.com/p4lang/p4c/tree/main/bazel/example) for an example of how to use or extend P4C in
your own Bazel project. You may use it as a template to get you started.

## Build system

The build system is based on cmake.  This section describes how it can be customized.

### Defining new CMake targets

When building a new backend target, add it into the development tree in the
extensions subdirectory. The cmake-based build system will automatically include
it if it contains a CMakeLists.txt file.

For a new backend, the cmake file should contain the following rules:

#### IR definition files

Backend specific IR definition files should be added to the global list
of IR_DEF_FILES as they are processed together with the core IR files.
Use the following rule:

```
set (IR_DEF_FILES ${IR_DEF_FILES} ${MY_IR_DEF_FILES} PARENT_SCOPE)
```
where `MY_IR_DEF_FILES` is a list of file names with absolute path
(for example, use `${CMAKE_CURRENT_SOURCE_DIR}`).

If in addition you have additional supporting source files, they
should be added to the ir sources, as follows:

```
set(EXTENSION_IR_SOURCES ${EXTENSION_IR_SOURCES} ${MY_IR_SRCS} PARENT_SCOPE)
```
Again, `MY_IR_SRCS` is a list of file names with absolute path.

#### Source files

Sources (.cpp and .h) should be added to the cpplint and clang-format target using the following rule:

```
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${MY_SOURCES_AND_HEADERS}")
add_clang_format_files (${CMAKE_CURRENT_SOURCE_DIR} "${MY_SOURCES_AND_HEADERS}")
```

Python files should be added to the black and isort target using the following rule:
```
add_black_files (${CMAKE_CURRENT_SOURCE_DIR} "${MY_SOURCES_AND_HEADERS}")
```

The P4C CMakeLists.txt will use that name to figure the full path of the files to lint.

clang-format, black, and isort need to be installed before the linter can be used. They can be installed with the following command:
```
uv pip install "clang-format==18.1.8" "black==24.3.0" "isort==5.13.2"
```
clang-format can be checked using the `make clang-format` command. Complaints can be fixed by running `make clang-format-fix-errors`. black and isort can be checked using the `make black` or `make isort` command respectively. Complaints can be fixed by running `make black-fix-errors` or `make isort-fix-errors`.

cpplint, clang-format, and black/isort run as checks as port of P4C's continuous integration process. To make sure that these tests pass, we recommend installing the appropriate git hooks. This can be done by running
```
./tools/install_git_hooks.sh
```
clang-format, cpplint, and black/isort checks will be enforced on every branch commit. In cases where checks are failing but the commit is sound, one can bypass the hook enforcement using `git commit --no-verify`.

#### Target

Define a target for your executable. The target should link against
the core `P4C_LIBRARIES` and `P4C_LIB_DEPS`.  `P4C_LIB_DEPS` are
package dependencies. If you need additional libraries for your
project, add them to `P4C_LIB_DEPS`.

In addition, your target should depend on the `genIR` target, since
you need all the IR generation to happen before you start compiling
your backend. If you chose to have your backend as a library (seem the
backends/bmv2 example), the library should depend on `genIR`, and
there is no longer necessary for your executable to depend on it.

```
add_executable(p4c-mybackend ${MY_SOURCES})
target_link_libraries (p4c-mybackend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-mybackend genIR)
```

#### Tests

We implemented support equivalent to the automake `make check` rules.
All tests should be included in `make check` and in addition, we support
`make check-*` rules. To enable this support, add the following rules:

```
set(MY_DRIVER <driver or compiler executable>)

set (MY_TEST_SUITES
  ${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4
  ${P4C_SOURCE_DIR}/testdata/p4_16_errors/*.p4
  )
set (MY_XFAIL_TESTS
  testdata/p4_16_errors/this_test_fails.p4
 )
p4c_add_tests("mybackend" ${MY_DRIVER} "${MY_TEST_SUITES}" "${MY_XFAIL_TESTS}")
```

In addition, you can add individual tests to a suite using the following macro:
```
set(isXFail FALSE)
set(SWITCH_P4 testdata/p4_14_samples/switch_20160512/switch.p4)

p4c_add_test_with_args ("mybackend" ${MY_DRIVER} ${isXFail}
  "switch_with_custom_profile" ${SWITCH_P4} "-DCUSTOM_PROFILE")
```

See the documentation for
[`p4c_add_test_with_args`](cmake/P4CUtils.cmake) and
[`p4c_add_tests`](cmake/P4CUtils.cmake) for more information on the
arguments to these macros.

To pass custom arguments to P4C, you can set the environment variable `P4C_ARGS`:
```
make check P4C_ARGS="-Xp4c=MY_CUSTOM_FLAG"
```

When making changes to P4C, it is sometimes useful to be able to run
the tests while overwriting the expected output files that are saved
in this repository.  One such situation is when your changes to P4C
cause the names of compiler-generated local variables to change.  To
force the expected output files to be rewritten while running the
tests, assign a value to the shell environment variable
`P4TEST_REPLACE`.  Here is one example Bash command to do so:

```
P4TEST_REPLACE=1 make check
```

#### Installation

Define rules to install your backend. Typically you need to install
the binary, the additional architecture headers, and the configuration
file for the P4C driver.

```
install (TARGETS p4c-mybackend
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.mybackend.cfg
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4c_src)
```

<!--!
\include{doc} "../lib/README.md"
-->
## Known issues

Issues with the compiler are tracked on
[GitHub](https://github.com/p4lang/p4c/issues). Before opening a new
issue, please check whether a similar issue is already opened. Opening
issues and submitting a pull request with fixes for those issues is
much appreciated.

In addition to the list of issues on Github, there are a number of
currently unsupported features listed below:

### Frontend

#### P4_14 features not supported in P4_16

* extern/blackbox attributes -- there is support for carrying them in
the IR, but they are lost if P4_16 code is output.  Backends can
access them from the IR

* Nonstandard extension primitives from P4_14
  * Execute_meter extra arguments
  * Recirculate/clone/resubmit variants
  * Bypass_egress
  * Sample_ primitives
  * invalidate

* No support for P4_14 parser exceptions.

### Backends

#### Bmv2 Backend

* Tables with multiple apply calls

See also [unsupported P4_16 language features](backends/bmv2/README.md#unsupported-p4_16-language-features).

## How to Contribute

We welcome and appreciate new contributions. Please take a moment to review our [Contribution Guidelines](CONTRIBUTING.md) to get started.

## P4 Compiler Onboarding
Educational material on P4: 

- General hands-on [tutorials](https://github.com/p4lang/tutorials).
- [Technical documentation on P4-related topics](https://github.com/jafingerhut/p4-guide?tab=readme-ov-file#introduction).
- Motivating P4: [IEEE ICC 2018 // Keynote: Nick McKeown, Programmable Forwarding Planes Are Here To Stay](https://www.youtube.com/watch?v=8ie0FcsN07U)
- Introducing P4-16 in detail: 
  - Part 1: [Introduction to P4_16. Part 1](https://www.youtube.com/watch?v=GslseT4hY1w)
  - Part 2: [Introduction to P4_16. Part 2](https://www.youtube.com/watch?v=yqxpypXIOtQ)
- Material on the official P4 compiler: 
  - [Understanding the Open-Source P4-16 Compiler - February 15, 2022 - Mihai Budiu](https://www.youtube.com/watch?v=Rx5AQ0IF6eU)
  - [Understanding P4-16 Open-Source Compiler, Part 2 - March 1, 2022 - Mihai Budiu](https://www.youtube.com/watch?v=YnPHPaPSmpU)
  - [Compiler Design - Implementation Architecture](https://github.com/p4lang/p4c/blob/main/docs/compiler-design.pdf).
- Introduction to P4Runtime: [Next-Gen SDN Tutorial - Session 1: P4 and P4Runtime Basics](https://www.youtube.com/watch?v=KRx92qSLgo4)

## Contact
We appreciate your contributions and look forward to working with you to improve the P4 Compiler Project (P4C)!
- For further assistance or questions regarding contributions, reach out to us in our [community chat](https://p4lang.zulipchat.com/).  [Joining link](https://p4lang.zulipchat.com/join/kjtv2reafrylssget425wx6c/) .
- For general P4-related questions, use the [P4 forum](https://forum.p4.org/).
- For other communication channels click [here](https://p4.org/join/).

