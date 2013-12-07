#!/usr/bin/env python

config_filename = "carbon_sim.cfg"
results_dir = "./tools/regress/simulation_results"

# Do not use 'localhost' or '127.0.0.1', use the machine name
machines = [
    "cagnode1",
    "cagnode2",
    "cagnode3",
    "cagnode4",
    "cagnode5",
    "cagnode6",
    ]

splash2_list = [
      "fft",
      "radix",
      "lu_contiguous",
      "lu_non_contiguous",
      "cholesky",
      "barnes",
      "ocean_contiguous",
      "ocean_non_contiguous",
      "water-nsquared",
      "water-spatial",
      "raytrace",
      "volrend",
      "radiosity",
      ]

parsec_list = [
      "blackscholes",
      "swaptions",
      "canneal",
      "fluidanimate",
      "streamcluster",
      "facesim",
      "freqmine",
      "dedup",
      "ferret",
      "bodytrack",
      ]

#benchmark_list = splash2_list + parsec_list
benchmark_list = splash2_list

lite_mode_list = [
      "freqmine",
      "dedup",
      "ferret",
      "bodytrack",
      ]

app_flags_map = {
      }

num_machines_list = [1,2]
