#!/bin/bash
cd pkgs/apps
for f in blackscholes bodytrack ferret fluidanimate freqmine raytrace vips x264
do
 cd $f
 mkdir run
 cd run
 tar xvf ../inputs/input_simmedium.tar
 cd ../..
done
cd ../kernels
for f in canneal dedup
do
 cd $f
 mkdir run
 cd run
 tar xvf ../inputs/input_simmedium.tar
 cd ../..
done
