#!/bin/sh

TOPDIR=`pwd`

make -f $TOPDIR/../buildroot_patch/tools/post-buildroot.mk \
 -C $TOPDIR/../buildroot_patch/ \
 TOPDIR=$TOPDIR


