# Singular/GPI-Space Framework

The Singular/GPI-Space Framework aims at massively parallel computations in computer algebra. It uses the computer algebra system [Singular](https://www.singular.uni-kl.de/) in conjunction with the workflow management system [GPI-Space](http://www.gpi-space.de/). While Singular provides the user frontend and computatinal backend, the coordination of the computations is done with GPI-Space. Parallel algorithms are modeled in terms of Petri nets.

The Singular/GPI-Space Framework has been used, for example, to determine smoothness of algebraic varieties, to compute GIT-fans, to compute tropicalizations of algebraic varieties, and to compute integration-by-parts relations for Feynman integrals in high energy physics.

For more information, see the webpage of the [Singular/GPI-Space Framework](https://www.mathematik.uni-kl.de/~boehm/singulargpispace/).


With an [open source version of GPI-Space](https://github.com/cc-hpc-itwm/gpispace) now available, this repository is the ongoing effort to make our code publicly available, this being the first version and part of the code. It contains simple programming primitives and as an example the code to test smoothnes of algebraic varieties. For details on the smoothness test, please refer to the paper [Towards Massively Parallel Computations in Algebraic Geometry](https://link.springer.com/article/10.1007/s10208-020-09464-x), Found. Comput. Math. (2020).

If you use the Singular/GPI-Space framework, please cite the paper

J. Böhm, W. Decker, A. Frühbis-Krüger, F.-J. Pfreundt, M. Rahn, L. Ristau: [Towards Massively Parallel Computations in Algebraic Geometry](https://link.springer.com/article/10.1007/s10208-020-09464-x), Found. Comput. Math. (2020).

as well as [Singular](https://www.singular.uni-kl.de/), [GPI-Space](http://www.gpi-space.de/), and the respective applications.

In the following we give detailed step-by-step instructions how to describe how to install the Singular/GPI-Space framework on a Linux System and give an example how to use it the smoothness test. This includes the installation of Singular, GPI-Space and of the necessary dependencies (for more details on this, please also refer to the respective web pages and repositories).

GPI-Space currently actively supports and tests the following Linux distributions:
* Centos 6
* Centos 7
* Centos 8
* Ubuntu 18.04 LTS
* Ubuntu 20.04 LTS

Similar distributions should also work. We occasionally also build on Gentoo. If you want to install on larger (pools of) machines, Centos is a good choice.


## Source and installation directories

Set directories for the build process and the install directory. The first one should typically be a fast local directory while the latter should be a (network) directory which is accessible from all machines involved in using GPI-Space.

```bash
mkdir /tmpbig/singular-gpispace
mkdir /scratch/singular-gpispace
export build_ROOT=/tmpbig/singular-gpispace
export install_ROOT=/scratch/singular-gpispace
```

## Installation of flint:

We install the arithmetics and number theory library flint, which is used by Singular.

```bash
cd ${build_ROOT}
mkdir flint
cd flint
git clone https://github.com/wbhart/flint2.git flint2
cd flint2
./configure --with-gmp=/usr --prefix=${build_ROOT}/tmp --with-mpfr=/usr
make -j $(nproc)
make install
export LD_LIBRARY_PATH=${build_ROOT}/tmp/lib
```
Note that for future use you should set LD_LIBRARY_PATH in your .profile or alike of your shell, since this environment variable will be needed for executing Singular.

## Installation of Singular:

We install the current version of Singular, which will be required by our framework.

Besides flint, Singular has various more standard dependencies, which are usually available through the package manager of your distribution. Please refer to the  [step-by-step instructions to build Singular](https://github.com/Singular/Singular/wiki/Step-by-Step-Installation-Instructions-for-Singular) for more details.

```bash
cd ${build_ROOT}
git clone --jobs $(nproc) https://github.com/Singular/Singular.git
cd Singular
./autogen.sh
mkdir ${build_ROOT}/sgbuild
cd ${build_ROOT}/sgbuild
${build_ROOT}/Singular/configure --with-flint=${build_ROOT}/tmp --prefix=${install_ROOT}/Singular420 --disable-static CXXFLAGS="-fno-gnu-unique"
make -j $(nproc)
make install
```



## Install Boost:

We install boost, which will be required by GPI-Space:

```bash
cd ${build_ROOT}
wget https://jztkft.dl.sourceforge.net/project/boost/boost/1.63.0/boost_1_63_0.tar.bz2
tar xf boost_1_63_0.tar.bz2
cd boost_1_63_0
./bootstrap.sh --prefix=${build_ROOT}/boost
./b2 -j$(nproc) --ignore-site-config cflags="-fPIC -fno-gnu-unique" cxxflags="-fPIC -fno-gnu-unique" link=static variant=release install
```


## Install libssh:

We install libssh since GPI-Space may run into problems on some systems with the ssh installation provided with the system.

```bash
cd ${build_ROOT}
arch=$(getconf LONG_BIT)
export PKG_CONFIG_PATH="${install_ROOT}/libssh/lib${arch}/pkgconfig"${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}
export LD_LIBRARY_PATH="${install_ROOT}/libssh/lib${arch}"${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
git clone --jobs $(nproc) --depth 1 --shallow-submodules --recursive --branch libssh2-1.9.0 https://github.com/libssh2/libssh2.git
cmake -D CRYPTO_BACKEND=OpenSSL -D CMAKE_BUILD_TYPE=Release -D ENABLE_ZLIB_COMPRESSION=ON -D BUILD_SHARED_LIBS=ON   -D CMAKE_INSTALL_PREFIX=${install_ROOT}/libssh -B libssh2/build -S libssh2
cmake --build libssh2/build --target install -j $(nproc)
```


## Install GPI-2:

We install GPI-2, which will be used by GPI-Space:

```bash
arch=$(getconf LONG_BIT)
mkdir ${install_ROOT}/GPI2
export GASPI_ROOT=${install_ROOT}/GPI2
export PKG_CONFIG_PATH="${GASPI_ROOT}/lib${arch}/pkgconfig"${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}
gpi2_version=1.3.2
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
```


## Install GPI-Space

We install GPI-Space version 20.12, which will be used by our framework.

Besides boost, GPI-2, and libssh, it has various more standard dependencies, which are usually available through the package manager of your distribution. Please refer to the installation instructions of the [open source version of GPI-Space](https://github.com/cc-hpc-itwm/gpispace) for more details.

```bash
gpispace_version=20.12
cd ${build_ROOT}
git clone --branch v${gpispace_version} https://github.com/cc-hpc-itwm/gpispace 
export GPISPACE_SOURCE_DIR=${build_ROOT}/gpispace
export GPISPACE_BUILD_DIR=${build_ROOT}/gpispace_build
export GPISPACE_INSTALL_DIR=${install_ROOT}/gpispace
export GPISPACE_TEST_DIR=${install_ROOT}/gpispace_test
export BOOST_ROOT=${build_ROOT}/boost
cd "${GPISPACE_SOURCE_DIR}"
mkdir -p "${GPISPACE_BUILD_DIR}" 
cd "${GPISPACE_BUILD_DIR}"
cmake -C ${GPISPACE_SOURCE_DIR}/config.cmake                      \
      -DCMAKE_INSTALL_PREFIX=${GPISPACE_INSTALL_DIR}              \
      -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=OFF                      \
      -B ${GPISPACE_BUILD_DIR}                                    \
      -S ${GPISPACE_SOURCE_DIR}
cmake --build ${GPISPACE_BUILD_DIR}                               \
      --target install                                            \
      -j $(nproc)
```


## Test GPI-Space:

The following is a short test for GPI-Space. It also creates a nodefile, which will be used in the following example. This file contains just the names of the computers used for computations with our framwork. In the example it just contains the result of hostname.

```bash
cd "${GPISPACE_BUILD_DIR}"
hostname > nodefile
export GSPC_NODEFILE_FOR_TESTS="${PWD}/nodefile"
```

Optionally: To test in a cluster allocation using the following systems (if available) do
* Slurm: export GSPC_NODEFILE_FOR_TESTS="$(generate_pbs_nodefile)"
* PBS/Torque: export GSPC_NODEFILE_FOR_TESTS="${PBS_NODEFILE}"

In any case then execute:

```bash
ctest --output-on-failure                                         \
      --tests-regex share_selftest
```





## Install the Singular/GPI-Space framework:

```bash
cd ${build_ROOT}
git clone git@github.com:singular-gpispace/framework.git
for i in cmake src/util-generic src/fhg/util/boost/program_options
do
  mkdir -p framework/${i}
  cp -R gpispace/${i}/* framework/${i}
done
cmake -DCMAKE_INSTALL_PREFIX=${install_ROOT}/framework -DCMAKE_BUILD_TYPE=Release -DGSPC_HOME=${GPISPACE_INSTALL_DIR} -DSINGULAR_HOME=${install_ROOT}/Singular420 -DINSTALL_DO_NOT_BUNDLE=ON -B frameworkbuild -S framework
cmake --build frameworkbuild --target install -j $(nproc)
```


## We try out the smoothness test.

The following should be present in ```bash ${install_ROOT}```:
* A nodefile with the machines to use (we copy the one which we created when testing GPI-Space)
* The file campedelli.sing with the example ideal which we will check for smoothness (this file can be found in the directory framework/smoothness-test/examples).

We thus do:

```bash
cd ${install_ROOT}
cp ${GPISPACE_BUILD_DIR}/nodefile .
cp ${build_ROOT}/framework/examples/smoothness-test/campedelli.sing .
```


Moreover, we need a directory for temporary files, which should be accessible from all machines involved in the computation:

```bash
mkdir ${install_ROOT}/temp
```



Optionally: We start the GPI-Space Monitor (to do so, you need an X-Server running).
In case you do not want to use the Monitor, you should not in Singular set the fields gc.options.loghost and gc.options.logport in the GPI-Space configuration token.

```bash
${install_ROOT}/gpispace/bin/gspc-monitor --port 9876
```

We start Singular and tell it where to find the library and the so file for the smoothness test:

```bash
cd ${install_ROOT}
SINGULARPATH=${install_ROOT}/framework ${install_ROOT}/Singular420/bin/Singular
```


In Singular do what follows below.

This 
* loads the library giving access to the smoothness test, 
* loads an example ideal of a Campedelli surface, then 
* creates a configuration token for the Singular/GPI-Space framework, 
  * adds information where to store temporary data, 
  * and where to find the nodefile, and 
  * sets how many processes per node should be started (usually one per core, not taking hyper-threading into account, you may have to adjust according to your hardware),
* creates a configuration token for the smoothness test,  
  * adds information on whether the input ideal is to be considered in projective space (or affine space), 
  * at which codimension the descent with Hironaka's criterion should stop and the standard Jacobian cirterion should be used, and finally 
* starts the computation.

Note that in more fancy environments like a cluster, one should specify absolute paths to the nodefile and the temp directory.

Note that this computation can take a while, a minute or so on 16 workers. It should return 1 for true. 

```bash
LIB "smoothtestgspc.lib";
< "campedelli.sing";
configToken gc = configure_gspc();
gc.options.tmpdir = "temp";
gc.options.nodefile = "nodefile";
gc.options.procspernode = 16;
configToken sc = configure_smoothtest();
sc.options.projective = 1;
sc.options.codimlimit = 2;
def result = smoothtest(I,gc,sc);
```

