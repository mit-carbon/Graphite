#!/usr/bin/env python

# Do not use 'localhost' or '127.0.0.1', use the machine name
machines = [
    "cagnode1",
    "cagnode2",
    "cagnode3",
    "cagnode4",
    "cagnode5",
    "cagnode6",
    ]

results_dir = "./tools/regress/simulation_results"

benchmark_list = [
      "fft",
      "radix",
      "lu_contiguous",
      "lu_non_contiguous",
      "cholesky",
      "barnes",
      "fmm",
      "ocean_contiguous",
      "ocean_non_contiguous",
      "water-nsquared",
      "water-spatial",
      "raytrace",
      "volrend",
      "radiosity",
      ]

command_map = {
      "fft"                   :"./tests/benchmarks/fft/fft -p64 -m20",
      "radix"                 :"./tests/benchmarks/radix/radix -p64 -n1048576",
      "lu_contiguous"         :"./tests/benchmarks/lu_contiguous/lu_contiguous -p64",
      "lu_non_contiguous"     :"./tests/benchmarks/lu_non_contiguous/lu_non_contiguous -p64 -n512",
      "cholesky"              :"./tests/benchmarks/cholesky/cholesky -p64 ./tests/benchmarks/cholesky/inputs/tk15.O",
      "barnes"                :"./tests/benchmarks/barnes/barnes < ./tests/benchmarks/barnes/input",
      "fmm"                   :"./tests/benchmarks/fmm/fmm < ./tests/benchmarks/fmm/inputs/input.16384",
      "ocean_contiguous"      :"./tests/benchmarks/ocean_contiguous/ocean_contiguous -p64",
      "ocean_non_contiguous"  :"./tests/benchmarks/ocean_non_contiguous/ocean_non_contiguous -p64",
      "water-nsquared"        :"./tests/benchmarks/water-nsquared/water-nsquared < ./tests/benchmarks/water-nsquared/input",
      "water-spatial"         :"./tests/benchmarks/water-spatial/water-spatial < ./tests/benchmarks/water-spatial/input",
      "raytrace"              :"./tests/benchmarks/raytrace/raytrace -p64 -m64 ./tests/benchmarks/raytrace/inputs/car.env",
      "volrend"               :"./tests/benchmarks/volrend/volrend 64 ./tests/benchmarks/volrend/inputs/head",
      "radiosity"             :"./tests/benchmarks/radiosity/radiosity -p 64 -batch -room",
      }

num_machines_list = [1,2]
