for i in $(find . -name ARG\*) ; do echo $i ; cd $i && python ~/research/sim/tools/scripts/progress_trace.py > data.m && math < ~/research/sim/tools/scripts/progress_trace.m && cd .. ; done
