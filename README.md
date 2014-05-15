# OSCCAR
## Introduction
OSCCAR is an suite of open source tools geared towards setting up, running and post-processing CFD simulations for industrial applications. OSCCAR builds on OpenFOAM®[1] and uses other various open source tools. The OSCCAR package includes tools, solvers, tutorials and documentation developed at an up-to-date code base of OpenFOAM. Ongoing contributions from the Department of Particulate Flow Modelling (PFM) at the Johannes Kepler University Linz, Austria, are added regularly. In addition, contributions from our academic and industrial partners are included in OSCCAR.

Please note that this project is work in progress. Any comments, ideas and suggestions are very welcome.

## Getting and installing OSCCAR
### Prerequisites
Most of the people involved use Ubuntu linux. Therefore, this package is primarily set up for Ubuntu 12.04. OSCCAR can be compiled on any linux system, but if you need help and/or have requests, please let us know.
To be able to get started a few dependencies are needed to compile OpenFOAM. These dependencies are the same as mentioned on the [OpenFOAM website](http://www.openfoam.org/download/git.php) plus octave and doxygen:
```bash
sudo apt-get install \
git build-essential flex bison cmake zlib1g-dev qt4-dev-tools libqt4-dev \
gnuplot libreadline-dev libncurses-dev libxt-dev libscotch-dev libopenmpi-dev \
libcgal-dev octave3.2 doxygen
```
To compile OpenFOAM-2.3.x it is useful to install gcc-4.8. Of course you can compile gcc-4.8 or clang yourself, but for the less adventurous among us, you can also install it [via a backport](http://askubuntu.com/questions/271388/how-to-install-gcc-4-8-in-ubuntu-12-04-from-the-terminal):
```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install gcc-4.8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50
```

### Pull the code and set up the environment
OSCCAR can be downloaded using git (a recent version of git is advisable). A default clone of the project stores the code in a directory named "OSCCAR" in the directory you issue the clone command from. OSCCAR assumes you have a directory in your home directory named "OSCCAR", where the package will be located. To get OSCCAR, simply do:
```bash
# Make sure you are in your home directory
cd

# Pull OSCCAR (do this in $HOME/OSCCAR)
git clone https://github.com/OSCCAR-PFM/OSCCAR.git
```
Of course you can change all this, this is vanilla installation. 

Now make sure your shell knows about OSCCAR and its environment variables by sourcing the OSCCAR environment. To do this, open up your bash startup file:
```bash
# Open .bashrc (note the dot before the file name!)
gedit ~/.bashrc
```
add the following line:
```bash
source $HOME/OSCCAR/OSCCAR/etc/bashrc
```
... and save and close the file.

To use the OSCCAR environment you need to make the shell reread ~/.bashrc again by either typing
```bash
source ~/.bashrc
```
... or closing the terminal and opening a new terminal.

### Compile OpenFOAM and OSCCAR
Now the environment has been set up OpenFOAM and OSCCAR can be compiled. Either change to the OSCCAR directory "by hand":
```bash
cd ~/OSCCAR
```
or use the OSCCAR alias:
```bash
osccar
```
In the OSCCAR-PFM directory there is a build file named Allwmake, after OpenFOAM's build scripts. Start the build by executing the Allwmake script:
```bash
./Allwmake
```

### Compiling in parallel
If you have multiple cores available compiling will be faster on more than a single core. You can set up the environment for parallel compiling by setting OpenFOAM's variable WM_NCOMPPROCS to the number of cores you have available. For example, in case of a four core machine, open up your ~/.bashrc and export WM_NCOMPPROCS:
```bash
# Open your bash startup file
gedit ~/.bashrc
```
... and add the following:
```bash
export WM_NCOMPPROCS=4
```
Note that after this change you need to re-source ~/.bashrc or open a new terminal for the environment variable to take effect.

## Using OSCCAR
The OpenFOAM and OSCCAR executables can be run anywhere on your system, as long as the current directory is a valid case directory. However, to keep things nice and tidy it is recommendable to follow OpenFOAM's directory structure and run cases in the run directory of your installation. After successful compilation do:
```bash
mkdir -p $OSCCAR_RUN
```
After this you can change to the run directory by typing:
```bash
run
```



## Disclamer
[1] This offering is not approved or endorsed by OpenCFD Limited, the producer of the OpenFOAM software and owner of the OPENFOAM®  and OpenCFD®  trade marks.
