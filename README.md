# Singular/GPI-Space Framework

The Singular/GPI-Space Framework aims at massively parallel computations in computer algebra. It uses the computer algebra system [Singular](https://www.singular.uni-kl.de/) in conjunction with the workflow management system [GPI-Space](http://www.gpi-space.de/) developed by Fraunhofer ITWM. While Singular provides the user frontend and computational backend, the coordination of the computations is done by GPI-Space. Parallel algorithms are modeled in the coordination layer in terms of Petri nets.

The Singular/GPI-Space Framework has been used, for example, to determine smoothness of algebraic varieties, to compute GIT-fans, to compute tropicalizations of algebraic varieties, and to compute integration-by-parts relations for Feynman integrals in high energy physics.

For more information, see the webpage of the [Singular/GPI-Space Framework](https://www.mathematik.uni-kl.de/~boehm/singulargpispace/).


With an [open source version of GPI-Space](https://github.com/cc-hpc-itwm/gpispace) now available, this repository is the ongoing effort to make our code publicly available, this being the second main version and part of the code. It contains basic infrastructure and, as an example, the code to test smoothness of algebraic varieties. Moreover, it contains convenient predefined workflows for wait all and wait first algorithmic structures.

For details on the smoothness test, please refer to the papers [Towards Massively Parallel Computations in Algebraic Geometry](https://link.springer.com/article/10.1007/s10208-020-09464-x), Found. Comput. Math. (2020).

If you use the Singular/GPI-Space framework, please cite the paper

J. Böhm, W. Decker, A. Frühbis-Krüger, F.-J. Pfreundt, M. Rahn, L. Ristau: [Towards Massively Parallel Computations in Algebraic Geometry](https://link.springer.com/article/10.1007/s10208-020-09464-x), Found. Comput. Math. (2020).

as well as [Singular](https://www.singular.uni-kl.de/), [GPI-Space](http://www.gpi-space.de/), and the respective applications.

To use the Singular/GPI-Space framework, it is necessary to install Singular, GPI-Space, their dependencies and the project code itself on a linux system. We will provide two different ways of installation. The preferred way is to use the package manager [Spack](https://spack.io/), which will take care of the dependencies automatically. In the following, we will describe how to install Spack and the framework and give examples how to use it, considering the smoothness test, wait all and wait first. A manual installation of all components is also possible, which can be used if an installation with Spack in not possible or not desirable. We will also give detailed step-by-step instructions for this approach, including the installation of Singular, GPI-Space and of the necessary dependencies (for more details on this, please also refer to the respective web pages and repositories).

GPI-Space currently actively supports and tests the following Linux distributions:
* Centos 7
* Centos 8
* Ubuntu 18.04 LTS
* Ubuntu 20.04 LTS

Similar distributions should also work. We occasionally also build on Gentoo. If you want to install on larger (pools/clusters of) machines, Centos is a good choice.

## Installation with Spack

We will assume that the user has some directory path to which she/he can read and write. In the following, we assume this path is set as the bash variable `software_ROOT`:

```bash
export software_ROOT=<software-root>

```
Note, this needs to be set for every new terminal session that the user wants to use for this project (or you must make sure it is set automatically).

Assuming Spack is not already installed, clone it from GitHub:

```bash
git clone https://github.com/spack/spack.git $software_ROOT/spack
```

To be able to use Spack in the current terminal session, run the setup script (this must be done for every new session):

```bash
. $software_ROOT/spack/share/spack/setup-env.sh
```

Next, Spack still needs to bootstrap clingo. While this is done automatically as soon as it is needed, we let Spack do it now by conretizing any spec (this may take a while).

```bash
spack spec zlib
```

We now clone our spack-packages repository ...

```bash
git clone https://github.com/singular-gpispace/spack-packages.git \
    $software_ROOT/spack-packages
```

... and add it to the Spack installation:

```bash
spack repo add $software_ROOT/spack-packages
```

Finally, we can install the framework:

```bash
spack install framework
```

Note that the initial installation may take quite a bit of time as all dependencies have to be built. To save some time, you could decide to build GPI-Space without the `gspc-monitor`, the graphical monitoring tool. This eliminates the need to build and install Qt. For this, you can use the following command instead:

```bash
spack install framework ^gpi-space~monitor
```

To use the framework, you now just have to load the installed software:

```bash
spack load framework
```

This also sets all required environment variables, in particular, you do not have to set a `SINGULARPATH` explicitly and you can run Singular by just typing `Singular`.

The following sections describe the manual installation. If your installation via Spack has been completed, you can now continue with the example sections. Note that the instructions regarding nodefile, temporary directory and loghostfile have to be adapted accordingly.

## Source and installation directories

Set a directory for the build process and the install directory. The first one should typically be a fast local directory, while the latter should be a (network) directory which is accessible from all machines involved in using GPI-Space.

```bash
mkdir /tmpbig/singular-gpispace
mkdir /scratch/singular-gpispace
export build_ROOT=/tmpbig/singular-gpispace
export install_ROOT=/scratch/singular-gpispace
```

Note that, for future use, you should add the exports to your .profile file or alike of your shell, since the exports are required to run our framework.


## Install flint:

We install the arithmetics and number theory library flint, which is used by Singular.

```bash
cd ${build_ROOT}
mkdir flint
cd flint
git clone https://github.com/wbhart/flint2.git flint2
cd flint2
./configure --with-gmp=/usr --prefix=${install_ROOT}/flint --with-mpfr=/usr
make -j $(nproc)
make install
export LD_LIBRARY_PATH=${install_ROOT}/flint/lib
```
Note that, for future use, you should set LD_LIBRARY_PATH in your .profile file or alike of your shell, since this environment variable will be needed for executing Singular.

Furthermore, note that flint is being actively developed, which results in changes to
the API; this can at times lead to the compilation of Singular to fail.  Should
this happen, rather compile a release version from flintlib.org as a workaround.
This can be done by replacing the step above `git clone https://github.com/wbhart/flint2.git flint2`
by the following:
```bash
wget http://www.flintlib.org/flint-2.7.1.tar.gz
tar -xf flint-2.7.1.tar.gz
cd flint-2.7.1
```
Then continue as before from the `./configure ...` line.


## Install Singular:

We install the current version of Singular, which will be required by our framework.

Besides flint, Singular has various more standard dependencies, which are usually available through the package manager of your distribution. Please refer to the Appendix of this readme and the [step-by-step instructions to build Singular](https://github.com/Singular/Singular/wiki/Step-by-Step-Installation-Instructions-for-Singular) for more details.

```bash
cd ${build_ROOT}
git clone --jobs $(nproc) https://github.com/Singular/Singular.git
cd Singular
./autogen.sh
mkdir ${build_ROOT}/sgbuild
cd ${build_ROOT}/sgbuild
${build_ROOT}/Singular/configure -C --with-flint=${install_ROOT}/flint --prefix=${install_ROOT}/Singular420 --disable-static CXXFLAGS="-fno-gnu-unique"
make -j $(nproc)
make install
```

## Install Boost:

We install boost, which will be required by GPI-Space:

```bash
cd ${build_ROOT}
export BOOST_ROOT=${build_ROOT}/boost

wget https://jztkft.dl.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.bz2
tar xf boost_1_63_0.tar.bz2
cd boost_1_63_0
./bootstrap.sh --prefix=${BOOST_ROOT}
./b2                               \
  -j $(nproc)                      \
  --ignore-site-config             \
  cflags="-fPIC -fno-gnu-unique"   \
  cxxflags="-fPIC -fno-gnu-unique" \
  link=static                      \
  variant=release                  \
  install
```

Please note that boost must be installed in a directory containing nothing else. You cannot use a system-provided boost directory.

## Install libssh:

We install libssh since GPI-Space may run into problems on some systems when using the ssh installation provided with the system.

```bash
cd ${build_ROOT}
Libssh2_ROOT=${install_ROOT}/libssh
libssh2_version=1.9.0

export PKG_CONFIG_PATH="${Libssh2_ROOT}/lib/pkgconfig:${Libssh2_ROOT}/lib64/pkgconfig"${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}

git clone                                \
  --jobs $(nproc)                        \
  --depth 1                              \
  --shallow-submodules                   \
  --recursive                            \
  --branch libssh2-${libssh2_version}    \
  https://github.com/libssh2/libssh2.git

cmake -D CRYPTO_BACKEND=OpenSSL                 \
      -D CMAKE_BUILD_TYPE=Release               \
      -D CMAKE_INSTALL_PREFIX="${Libssh2_ROOT}" \
      -D ENABLE_ZLIB_COMPRESSION=ON             \
      -D BUILD_SHARED_LIBS=ON                  \
      -D CMAKE_POSITION_INDEPENDENT_CODE=ON     \
      -D BUILD_TESTING=OFF                      \
      -B libssh2/build                          \
      -S libssh2
cmake --build libssh2/build --target install -j $(nproc)
```

## Install GPI-2:

We install [GPI-2](http://www.gpi-site.com/), an API for the development of scalable, asynchronous and fault tolerant parallel applications developed by Frauhofer ITWM, which will be used by GPI-Space.

```bash
cd ${build_ROOT}
mkdir ${install_ROOT}/GPI2
export GASPI_ROOT=${install_ROOT}/GPI2
gpi2_version=1.3.3
git clone                                                                 \
    --depth 1                                                             \
    --branch v${gpi2_version}                                             \
    https://github.com/cc-hpc-itwm/GPI-2.git                              \
    GPI-2
cd GPI-2
grep "^CC\s*=\s*gcc$" . -lR | xargs sed -i'' -e '/^CC\s*=\s*gcc$/d'
./install.sh -p "${GASPI_ROOT}"                                           \
             --with-fortran=false                                         \
             --with-ethernet
export PKG_CONFIG_PATH="${GASPI_ROOT}/lib64/pkgconfig"${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}
```

Remark: This code assumes that the system to be used does not use Infiniband.

## Install GPI-Space

We install GPI-Space version 22.06, which will be used by our framework.

Besides boost, GPI-2, and libssh, it has various, more standard, dependencies, which are usually available through the package manager of your distribution. Please refer also to the installation instructions of the [open source version of GPI-Space](https://github.com/cc-hpc-itwm/gpispace) for more details.

The follwing code assumes that the environment variables `BOOST_ROOT` (for Boost) and `PKG_CONFIG_PATH` (for libssh and GPI-2) are still set. It, moreover, assumes that the unit and system tests of GPI-Space are built.

Remark: If the installation process seems to prefer a system-wide installed boost, you can set the CMake option `-D Boost_NO_BOOST_CMAKE=on` (see also the GPI-Space documentation).

Remark: If you want to save compilation time, you can install GPI-Space without testing enabled by not setting `build_tests`.

```bash
cd ${build_ROOT}
gpispace_version=22.06
build_tests="-DBUILD_TESTING=on -DSHARED_DIRECTORY_FOR_TESTS=${install_ROOT}/gspctest"
export GPISpace_ROOT=${install_ROOT}/gpispace

wget "https://github.com/cc-hpc-itwm/gpispace/archive/v${gpispace_version}.tar.gz" \
  -O "gpispace-v${gpispace_version}.tar.gz"
tar xf "gpispace-v${gpispace_version}.tar.gz"
export GPISPACE_REPO=${build_ROOT}/gpispace-${gpispace_version}

cmake -D CMAKE_INSTALL_PREFIX="${GPISpace_ROOT}"  \
      -B "${GPISPACE_REPO}/build"                 \
      -S "${GPISPACE_REPO}"                       \
      ${build_tests:-}

cmake --build "${GPISPACE_REPO}/build"            \
      --target install                            \
      -j $(nproc)
```

Note that GPI-Space requires the ability to log into the computation nodes via ssh with ssh-key authorization. If you do not have this setup already, you should generate an ssh key and add it to the authorized-keys file. Make also sure that an ssh server is running. If you ssh to the machines you should not be asked for a password. The following code provides such a setup.

```bash
ssh-keygen -t rsa -C "your_email@example.com"
cd ~/.ssh
cat id_rsa.pub >> authorized_keys
```

## Test GPI-Space (optional):

The following is a short test for GPI-Space. It also creates a nodefile, which will be used in the subsequent examples. This file contains the names of the nodes used for computations with our framework. In the example, it just contains the result of hostname.

```bash
mkdir -p ${install_ROOT}/gspctest
cd ${install_ROOT}
hostname > nodefile
cd "${GPISPACE_REPO}/build"
export GSPC_NODEFILE_FOR_TESTS="${install_ROOT}/nodefile"
```

Optionally: To test in a cluster allocation using the following systems (if available) do
* Slurm: export GSPC_NODEFILE_FOR_TESTS="$(generate_pbs_nodefile)"
* PBS/Torque: export GSPC_NODEFILE_FOR_TESTS="${PBS_NODEFILE}"
On Slurm, the package providing `generate_pbs_nodefile` might have to be installed separately on the cluster.

In any case, then execute:

```bash
ctest --output-on-failure          \
      --tests-regex share_selftest
```

Remark: If you have installed GPI-Space without testing enabled, you can still verify the installation by running the "stochastic_with_heureka" example as a selftest (see GPI-Space's README.md for details).

## Install the Singular/GPI-Space framework:

We assume that the environment variable `GPISpace_ROOT` is set as above.

```bash
cd ${build_ROOT}

git clone https://github.com/singular-gpispace/framework.git
cmake -D CMAKE_INSTALL_PREFIX=${install_ROOT}/framework \
      -D CMAKE_BUILD_TYPE=Release                       \
      -D SINGULAR_HOME=${install_ROOT}/Singular420      \
      -D Boost_NO_BOOST_CMAKE=on                        \
      -B framework_build                                \
      -S framework
cmake --build framework_build --target install -j $(nproc)
```

## We try out the smoothness test.

To do so, a nodefile with the machines to use the following should be present in `${install_ROOT}` (assumed to be present from testing GPI-Space).
The files campedelli.sing or quadric.sing with example ideals which define varieties which we will check for smoothness have been installed to `${install_ROOT}/framework/share/examples/smoothness-test`.

Moreover, we need a directory for temporary files, which should be accessible from all machines involved in the computation:

```bash
mkdir ${install_ROOT}/temp
```

* Optionally: We start the GPI-Space Monitor (to do so, you need an X-Server running) to display computations in form of a Gantt diagram.
In case you do not want to use the monitor, you should not set in Singular the fields options.loghostfile and options.logport of the GPI-Space configuration token (see below). In order to use the GPI-Space Monitor, we need a loghostfile with the name of the machine running the monitor.

```bash
cd ${install_ROOT}
hostname > loghostfile
```

On this machine, start the monitor, specifying a port number where the monitor will receive information from GPI-Space. The same port has to be specified in Singular in the field options.logport.

```bash
${install_ROOT}/gpispace/bin/gspc-monitor --port 9876
```


We start Singular, telling it where to find the library and the so-file for the smoothness test:

```bash
cd ${install_ROOT}
SINGULARPATH=${install_ROOT}/framework ${install_ROOT}/Singular420/bin/Singular
```


In Singular do what follows below.

This
* loads the library giving access to the smoothness test,
* loads an example ideal of a Campedelli surface, then
* creates a configuration token for the Singular/GPI-Space framework,
  * adds information where to store temporary data (in the field options.tmpdir),
  * where to find the nodefile (in the field options.nodefile), and
  * sets how many processes per node should be started (in the field options.procspernode, usually one process per core, not taking hyper-threading into account; you may have to adjust according to your hardware),
* creates a configuration token for the smoothness test,
  * adds information on whether the input ideal is to be considered in projective space or affine space (in the field options.projective), and
  * at which codimension the descent with Hironaka's criterion should stop and the standard Jacobian criterion should be used (in the field options.codimlimit), and finally
* starts the computation.

Note that in more fancy environments like a cluster, one should specify absolute paths to the nodefile and the temp directory.

Note that this computation can take a while, a minute or so on 16 workers. It should return 1 for true. A shorter example for smaller machines is given below.

```bash
LIB "smoothtestgspc.lib";
< "share/examples/smoothness-test/campedelli.sing";
configToken gc = configure_gspc();
gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 16;
configToken sc = configure_smoothtest();
sc.options.projective = 1;
sc.options.codimlimit = 2;
def result = smoothtest(I,gc,sc);
```

The same computation including logging on the GPI-Space monitor. Here, we have to specify where to find the loghostfile (in the field options.loghostfile), and what is the port of the monitor accepting connections (in the field options.logport, in our example, this was 9876). One can alternatively also specify the loghost directly via the field options.loghost (this option has priority over options.loghostfile).


```bash
LIB "smoothtestgspc.lib";
< "share/examples/smoothness-test/campedelli.sing";
configToken gc = configure_gspc();
gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 16;
gc.options.loghostfile = "loghostfile";
gc.options.logport = 9876;
configToken sc = configure_smoothtest();
sc.options.projective = 1;
sc.options.codimlimit = 2;
def result = smoothtest(I,gc,sc);
```

On a smaller machine, try the following, which should finish instantaneously:

```bash
LIB "smoothtestgspc.lib";
< "share/examples/smoothness-test/quadric.sing";
configToken gc = configure_gspc();
gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 2;
configToken sc = configure_smoothtest();
sc.options.projective = 1;
sc.options.codimlimit = 0;
def result = smoothtest(I,gc,sc);
```
The same including logging:

```bash
LIB "smoothtestgspc.lib";
< "share/examples/smoothness-test/quadric.sing";
configToken gc = configure_gspc();
gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 2;
gc.options.loghostfile = "loghostfile";
gc.options.logport = 9876;
configToken sc = configure_smoothtest();
sc.options.projective = 1;
sc.options.codimlimit = 0;
def result = smoothtest(I,gc,sc);
```

## Trying out the wait_all workflow

We demonstrate the use of the gspc_wait_all command, which applies a Singular procedure to multiple input data, waiting until all computations finish, and then returns all results.

The following example computes modulo a number of primes standard bases for an ideal defined over the rationals, that is, the ideal is reduced module the respective prime and the respective standard bases are computed. The results are then lifted to the rationals. This will lead to a standard basis over the rational provided we use sufficiently many good primes.

Note that this example is only for demonstration purposes and does not verify the results or dynamically chooses the number of primes. It, however, provides the option to do a plausibility check.

As in the previous example, some additional files are needed:
* A nodefile with the machines to use (which we assume to present from testing GPI-Space).
* The Singular library providing the procedure that is to be run inside the worker processes. In this example, the library also contains some procedures that are used in the user interface Singular session controlling the computation. For our example, we use the library `gspcmodstd.lib` that has been installed to `${install_ROOT}/framework/share/examples/wait-all-first` and is thus available on all nodes.

We need a directory for temporary files, which should be accessible from all machines involved in the computation and you might already have generated in the last example:

```bash
mkdir ${install_ROOT}/temp
```

* Optionally, the GPI-Space Monitor can be used to display computations in form of a Gantt diagram, see the previuos example on how to start it and how to prepare the loghostfile.
If you do not set the fields options.loghostfile and options.logport of the GPI-Space configuration token then the monitor is not utilized.

We start Singular, telling it where to find the library and the so-file:

```bash
cd ${install_ROOT}
SINGULARPATH=${install_ROOT}/framework ${install_ROOT}/Singular420/bin/Singular
```

The Singular/GPI-Space application containing the parallel-wait all is then loaded by:

```
LIB "parallelgspc.lib";
```


In the following code, a ideal I in a polynomial ring of characteristic zero with three variables is generated in a random way. Then, in parallel, the ideal is reduced modulo num_primes=8 different primes p_i by reducing its generators (the primes are chosen, using the function primeList() from modstd.lib, such that the reduction is well-defined, that is, the denominators are coprime to the all p_i). This is followed by a computation of the respective standard bases.
The Chinese Remainder Theorem is then applied to lift the result to $\mathbb{Z}/n$ with n the product of the p_i, and this result is lifted to the rationals using the Farey map. In order to allow for a plausibility check, only the first num_primes - 1 = 7 ideals are used. If the function liftIdeals is called with the additional argument 1, the result is compared to the one obtained from using num_primes=8 primes. If this is the case, the result is very likely a standard basis over the rationals (however this is not a proof).

Note that gspcmodstd.lib provides both the procedure used in the workers as some auxiliary procedures used in the user interface Singular. This is why we have to load it also in the user interface Singular. The library is being automatically loaded in the worker Singulars, as specified in the token pc.

```
LIB "random.lib";
LIB "modstd.lib";
LIB "share/examples/wait-all-first/gspcmodstd.lib";


configToken gc = configure_gspc();

gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 4;
gc.options.loghostfile = "loghostfile";
gc.options.logport = 9876;

configToken pc = configure_parallel();

pc.options.InTokenTypeName = "inputToken";
pc.options.InTokenTypeDescription = "ideal inputideal, int characteristic";
pc.options.OutTokenTypeName = "outputToken";
pc.options.OutTokenTypeDescription = "ideal outputideal";
pc.options.loadlib = system("getenv","SINGULARPATH") + "/share/examples/wait-all-first/gspcmodstd.lib";
pc.options.transitionProcedure = "stdmodp";

generateIOTokenTypes(pc);

ring R = 0,(x,y,z),lp;
ideal I = randomid(maxideal(2),2);
I;

inputToken ins;
ins.inputideal = I;

int num_primes = 8;
intvec primeiv = primeList(I, num_primes);

int i;
list listOfInputTokens;

for (i = 1; i <= num_primes; i++)
{
  ins.characteristic = primeiv[i];
  listOfInputTokens[i] = ins;
}

def re = gspc_wait_all(listOfInputTokens, gc, pc);

liftIdeals(re);

// with plausibility check
liftIdeals(re, 1);
```

## Trying out the wait_first workflow

We demonstrate the use of the gspc_wait_first command, which applies a Singular procedure to multiple input data, waiting until the first computation finishes, and then returns the corresponding result. The remaining computations are terminated.

In our example, a standard basis for a random ideal is computed with respect to different monomial orderings in parallel. As soon as the first standard basis computation finishes, the result of this computation is returned. In the example, we use the degree reverse lexicographical ordering ("dp") and the lexicographical ordering ("lp"). The ordering dp usually requires less computation time for the standard basis, while lp can be used for eliminating variables.

Note that the length of the integer vector which can be specified in addition to the name of the ordering should agree with the number of variables of the polynomial ring. In case of weighted orderings, one can specify in this vector integer weights for the variables.

We assume that the nodefile and the temporary directory have been created as described above and, optionally, the gspc-monitor has been started as described above.

As in the previous example, the library `gspccomputestd.lib` providing the Singular procedure to be used has been installed to `${install_ROOT}/framework/share/examples/wait-all-first`.

After starting Singular as described above, the following Singular code starts the computation. Note that the library gspccomputestd.lib is automatically loaded in the Singular worker processes as specified in the pc token.

```
LIB "random.lib";
LIB "parallelgspc.lib";

configToken gc = configure_gspc();

gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 4;
gc.options.loghostfile = "loghostfile";
gc.options.logport = 9876;

configToken pc = configure_parallel();

pc.options.InTokenTypeName = "inputToken";
pc.options.InTokenTypeDescription = "ideal I, string ordering, intvec v";
pc.options.OutTokenTypeName = "outputToken";
pc.options.OutTokenTypeDescription = "ideal I, string ordering, intvec v";
pc.options.loadlib = system("getenv","SINGULARPATH") + "/share/examples/wait-all-first/gspccomputestd.lib";
pc.options.transitionProcedure = "computeStd";

generateIOTokenTypes(pc);

ring r = 0,(x1,x2,x3,x4),dp;
ideal I = randomid(maxideal(4), 4, 4);
I;

list listOfInputTokens;
inputToken ins;
ins.I = I;
ins.ordering = "lp";
ins.v = intvec(1:4);
listOfInputTokens[1] = ins;
ins.ordering = "dp";
ins.v = intvec(1:4);
listOfInputTokens[2] = ins;


def re = gspc_wait_first(listOfInputTokens, gc, pc);

def S = re.r_I;
setring S;
re.I;
```

## Appendix: Standard packages required to build the framework

Assuming that we are installing on a Ubuntu system (analogous packages exist in other distributions), we give installation instructions for standard packages which are required by the framework and may not be included in your of-the-shelf installation.

Note that the following requires root privileges. If you do not have root access, ask your administator to install these packages. You may want to check with `dpkg -l <package name>` whether the package is installed already.

* Version control system Git used for downloading sources:
  ```bash
  sudo apt-get install git
  ```

* Tools necessary for compiling the packages:
  ```bash
  sudo apt-get install build-essential
  sudo apt-get install autoconf
  sudo apt-get install autogen
  sudo apt-get install libtool
  sudo apt-get install libreadline6-dev
  sudo apt-get install libglpk-dev
  sudo apt-get install cmake
  sudo apt-get install gawk
   ```

  Or everything in one command:
  ```bash
  sudo apt-get install build-essential autoconf autogen libtool libreadline6-dev libglpk-dev cmake gawk
  ```
  Remark: On Ubuntu 18.04, you will have to install a more recent version of CMake (>= 3.15). You can build CMake from source or download pre-compiled binaries via [CMake](https://cmake.org/).

* Scientific libraries used by Singular:
  ```bash
  sudo apt-get install libgmp-dev
  sudo apt-get install libmpfr-dev
  sudo apt-get install libcdd-dev
  sudo apt-get install libntl-dev
  ```

  Or everything in one command:
  ```bash
  sudo apt-get install libgmp-dev libmpfr-dev libcdd-dev libntl-dev
  ```

* Library required by to build libssh:
  ```bash
  sudo apt-get install libssl-dev
  ```

* Libraries required by GPI-Space
  ```bash
  sudo apt-get install openssh-server
  sudo apt-get install hwloc
  sudo apt-get install libhwloc-dev
  sudo apt-get install libudev-dev
  sudo apt-get install qt5-default
  sudo apt-get install chrpath
  ```

  Or everything in one command:
  ```bash
  sudo apt-get install openssh-server hwloc libhwloc-dev libudev-dev qt5-default chrpath
  ```
  Remark: On newer versions of Debian/Ubuntu, the Qt packages are named differently, you might have to install `qtbase5-dev`.
