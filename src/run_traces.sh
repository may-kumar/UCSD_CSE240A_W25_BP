#!/bin/bash
make clean
make
for file in ../traces/*.bz2
do
    echo "Using $file"
    bunzip2 -kc $file |  ./predictor $*
done