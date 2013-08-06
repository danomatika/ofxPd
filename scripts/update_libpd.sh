#! /bin/bash

WD=$(dirname $0)

SRC=libpd
DEST=../libs/libpd

###

cd $WD

# get latest source
git clone git://github.com/libpd/libpd.git

# remove uneeded makefiles
find $SRC -name "GNUmakefile.am" -delete
find $SRC -name "Makefile.am" -delete
find $SRC -name "makefile" -delete
rm $SRC/pure-data/extra/makefile.subdir

# we dont need the java or csharp wrappers
rm $SRC/libpd_wrapper/z_jni.c
rm $SRC/libpd_wrapper/z_jni.h
rm $SRC/libpd_wrapper/z_csharp_helper.c
rm $SRC/libpd_wrapper/z_csharp_helper.h

# remove expr~ since it's GPL, leave that up to devs
rm -rf $SRC/pure-data/extra/expr~
rm $SRC/pure-data/extra/expr-help.pd

# don't need the ringbuffer layer
rm $SRC/libpd_wrapper/util/ringbuffer.*
rm $SRC/libpd_wrapper/util/z_hook_util.*
rm $SRC/libpd_wrapper/util/z_queued.*

# copy license
cp -v $SRC/LICENSE.txt $DEST

# copy sources
cp -Rv $SRC/cpp $DEST
cp -Rv $SRC/pure-data $DEST
cp -Rv $SRC/libpd_wrapper $DEST

# cleanup
rm -rf $SRC

