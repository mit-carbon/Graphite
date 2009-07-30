#!/bin/bash

sed -i s/-O0/-O3/ common/Makefile.common
make clean && make
mv lib/pin_sim.so temp/
sed -i s/-O3/-O0/ common/Makefile.common
make clean && make
mv temp/pin_sim.so lib/
