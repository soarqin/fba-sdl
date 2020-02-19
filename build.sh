#!/bin/bash

SAVEDPATH=$PATH
export PATH=/opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/bin:/opt/gcw0-toolchain/usr/bin:/opt/gcw0-toolchain/usr/sbin:$PATH
make -f Makefile.dingux && pushd ./bin && ./make_opk.sh && popd
export PATH=$SAVEDPATH
