#!/bin/sh
echo "Starting: Regression test suite"
date
python -u tools/regress/run_tests.py > tools/regress/detailed.log 2>&1
python -u tools/regress/aggregate_results.py >> tools/regress/detailed.log 2>&1
cat tools/regress/summary.log >> tools/regress/detailed.log
echo ""
echo "Ending: Regression test suite"
date
echo ""
cat tools/regress/summary.log
echo ""
