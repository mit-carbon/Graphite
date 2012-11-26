#!/usr/bin/env python

# job info
# Do not use 'localhost' or '127.0.0.1', use the machine name
machines = [
    "cagnode6",
    "cagnode7",
    "cagnode8",
    "cagnode9"
    ]

results_dir = "./tools/regress/simulation_results"

benchmark_list = ["radix", "fft"]
command_list = [
      "./tests/benchmarks/radix/radix -p64",
      "./tests/benchmarks/fft/fft -p64 -m16"
      ]
num_machines_list = [1,2]
