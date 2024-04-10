#! /bin/sh

# exit on error
set -e

DEST=../libs/libpd

###

cd "$(dirname $0)"

rm -rfv $DEST/*
git checkout $DEST 
