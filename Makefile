include common/makefile.gnu.config

PIN_BIN=/afs/csail.mit.edu/group/carbon/tools/pin/current/pin
PIN_TOOL=pin/bin/pin_sim
PIN_RUN=mpirun -np 1 $(PIN_BIN) -mt -t $(PIN_TOOL) 
TESTS_DIR=./common/tests

all:
	$(MAKE) -C common 
	$(MAKE) -C pin
	$(MAKE) -C qemu

use-mpi:
	$(MAKE) use-mpi -C common/phys_trans
	$(MAKE) clean -C common/phys_trans

use-sm:
	$(MAKE) use-sm -C common/phys_trans
	$(MAKE) clean -C common/phys_trans

pinbin:
	$(MAKE) -C common
	$(MAKE) -C pin

qemubin:
	$(MAKE) -C common
	$(MAKE) -C qemu

clean:
	$(MAKE) -C common clean
	$(MAKE) -C pin clean
	$(MAKE) -C qemu clean
	-rm -f *.o *.d *.rpo

squeaky: clean
	$(MAKE) -C common squeaky
	$(MAKE) -C pin squeaky
	$(MAKE) -C qemu squeaky
	-rm -f *~

io_test: all
	$(MAKE) -C $(TESTS_DIR)/file_io
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/file_io/file_io

ping_pong_test: all
	$(MAKE) -C $(TESTS_DIR)/ping_pong
	$(PIN_RUN) -mdc -mpf -msys -n 2 -- $(TESTS_DIR)/ping_pong/ping_pong

pthread_test: all
	$(MAKE) -C $(TESTS_DIR)/pthreads_matmult
	$(PIN_RUN) -mdc -mpf -msys -n 64 -- $(TESTS_DIR)/pthreads_matmult/cannon -m 64 -s 64

love:
	echo "not war!"
