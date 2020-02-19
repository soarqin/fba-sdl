#!/bin/bash

SAVEDPATH=$PATH
export PATH=/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/bin:/opt/gcw0-toolchain/usr/bin:/opt/gcw0-toolchain/usr/sbin:$PATH
make -f Makefile.dingux clean
rm -f bin/fbasdl.dge
export PATH=$SAVEDPATH
