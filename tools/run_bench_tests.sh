#!/bin/sh
make regress_bench >&run_bench_tests.log
grep "^TEST:" run_bench_tests.log >run_bench_tests.tmp
cat run_bench_tests.tmp >>run_bench_tests.log
cat run_bench_tests.tmp
rm run_bench_tests.tmp

