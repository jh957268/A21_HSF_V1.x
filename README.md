# AKS Firmware for HF-A21-SMT

## Ubuntu Linux

### Sane build environment

Use `apt-get` to install the `dos2unix` utility.

> sudo apt-get install dos2unix

Use `apt-get` to install the 32-bit legacy libstdc++6 library.

> sudo apt-get install lib32stdc++6

The eCos 1.x tree uses legacy 32-bit binary tools.  Install a 32-bit version of `libstdc++5`

> sudo apt-get install libstdc++5:i386

### Installing eCos

> wget --passive-ftp ftp://ecos.sourceware.org/pub/ecos/ecos-install.tcl
>
> sudo sh ecos-install.tcl

*   Install eCos into /opt/ecos
*   Select `mipsisa32-elf` build toolchain.

Add the eCos build environment to your PATH

> echo '. /opt/ecos/ecosenv.sh' >> ~/.bashrc

Close and re-open your terminal to effect the PATH change.

<!--
Update the build tool to libstdc++6

> cd /opt/ecos

> sudo mv gnutools gnutools.libstdc++5

> sudo wget https://mirrors.kernel.org/sources.redhat.com/ecos/gnutools/i386linux/ecoscentric-gnutools-mipsisa32-elf-20081107-sw.i386linux.tar.bz2

> sudo tar -xvjf ecoscentric-gnutools-mipsisa32-elf-20081107-sw.i386linux.tar.bz2 -->


## Windows

1.  CygWin
1.  eCos
2.  more testing!

## Get the Source

Clone the git repository.

> git clone `https://github.com/RatPacDimmers/A21_HSF_V1.x.git`

You can now build the image by running these commands:

> cd A21_HSF_V1.x
>
> make clean
>
> make
