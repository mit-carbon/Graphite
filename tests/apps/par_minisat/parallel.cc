/******************************************************************************************[Main.C]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/


//#define DEBUG_PRINT


#include <unistd.h>
#include <ctime>
#include <signal.h>

#include <fstream>
#include <iostream>

#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "carbon_user.h"

using namespace std;


//#include "capi.h"


#include "Solver.h"


// TODO: remove global S variable. make non-global.
char* input_filename;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void printStats(SolverStats& stats)
{
    double  cpu_time = cpuTime();
    int64   mem_used = memUsed();
    reportf("restarts              : %"I64_fmt"\n", stats.starts);
    reportf("conflicts             : %-12"I64_fmt"   (%.0f /sec)\n", stats.conflicts   , stats.conflicts   /cpu_time);
    reportf("decisions             : %-12"I64_fmt"   (%.0f /sec)\n", stats.decisions   , stats.decisions   /cpu_time);
    reportf("propagations          : %-12"I64_fmt"   (%.0f /sec)\n", stats.propagations, stats.propagations/cpu_time);
    reportf("conflict literals     : %-12"I64_fmt"   (%4.2f %% deleted)\n", stats.tot_literals, (stats.max_literals - stats.tot_literals)*100 / (double)stats.max_literals);
    if (mem_used != 0) reportf("Memory used           : %.2f MB\n", mem_used / 1048576.0);
    reportf("CPU time              : %g s\n", cpu_time);
}


static void SIGINT_handler(int signum, Solver *solver){
    reportf("\n"); reportf("*** INTERRUPTED ***\n");
    printStats(solver->stats);
    reportf("\n"); reportf("*** INTERRUPTED ***\n");
    exit(1);
}


void parse_input(char* the_input_filename, Solver &S) {
	// we're only supporting uncompressed file reading. standard CNF format.


	
	ifstream in;
	reportf("Attempting to read file %s\n", the_input_filename);
	in.open(the_input_filename, ios::in);
	
	reportf("DONE Attempting to read file %s\n", the_input_filename);




	string line;
	vec<Lit> lits;
	int line_no = 0;
	while (getline(in, line) ) {



		//cout << line << endl;
		if (line[0] == 'c' || line[0] == 'p')
		{
			// skip comment
			continue;
		}
		else
		{
			// 1. read line from input file. split on whitespace and strip rest of whitespace.
			// lines look like this: -1232 -1298 -181 0 8
			
			vector<string> tokens;
			boost::split(tokens, line, boost::is_any_of("\t "));
			
			// here we parse the line into integers and build a vec<Lit> out of it
			vec<Lit> lits;
			int var;
			lits.clear();


			for(vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
				
				try
				{
					int parsed_lit = boost::lexical_cast< int >( *it );
					// DEBUG:
					//cout << "(" << *it << ")" << " --> " << parsed_lit << endl;	

					var = abs(parsed_lit) - 1;
					if (parsed_lit == 0) break;

#define GROW_VARS			
#ifdef GROW_VARS
					// problematic line!!!
					while (var >= S.nVars()) S.newVar();
#endif
					lits.push( (parsed_lit > 0) ? Lit(var) : ~Lit(var) );

				}
				catch( const boost::bad_lexical_cast & )
				{
					fprintf(stderr, "Bad input \"%s\". Exiting\n\n", *it);
					exit(-3000);
				}
			}


			
			// 3. add new clause to solver
			if(line_no % 100 == 0) cout << line_no << ".\t----- adding to solver -----" << endl;
			
#ifdef DEBUG_PRINT
			// psota hack: 
			printf("START NEW LIT VECTOR____\n");
			for(int i = 0; i < lits.size(); i++) {
				printf("%d.\t%d\n", i, index(lits[i]));
			}
			printf("END NEW LIT VECTOR____\n");
			// psota hack end
#endif


			S.addClause(lits);
		}
		line_no++;
	}


	
	in.close();

	
}


void* worker_main_simple(void *threadid) {
	/* Find out my identity */
	long rank = (long)threadid;
	reportf("worker_main_simple[%ld]: Hello from process id %ld\n", rank, rank);
	
	Solver S;
	
	reportf("thread[%ld]:\tparsing input...\n", rank);
	parse_input(input_filename, S);
	reportf("thread[%ld]:\tDONE parsing input...\n", rank);
	
#ifdef DEBUGME	
	reportf("thread[%ld]:\tChecking for unsatisfiability\n", rank);
    if (!S.okay()){
		reportf("Trivial problem\n");
        reportf("UNSATISFIABLE\n");
        exit(20);
    }
	reportf("thread[%ld]:\tDONE Checking for unsatisfiability\n", rank);
	
	
	reportf("thread[%ld]: Solving system\n", rank);
    S.verbosity = 1;
    solver = &S;
    signal(SIGINT,SIGINT_handler);
    signal(SIGHUP,SIGINT_handler);

    S.solve();
	reportf("thread[%ld]: DONE solving system\n", rank);
    printStats(S.stats);
    reportf("\n");
    reportf((S.okay() ? "thread[%ld]: SATISFIABLE\n" : "thread[%ld]: UNSATISFIABLE\n"), rank);

#endif
	

	reportf("thread[%ld]: DONE worker thread function\n", rank);

	// TODO: should I be returning something other than NULL here?
	return NULL;
}



int main(int argc, char** argv)
{
	// help message
	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
	{
		reportf("USAGE: %s <input-file> <result-output-file>\n  where the input may be either in plain/gzipped DIMACS format or in BCNF.\n", argv[0]);
		exit(0);
	} else {
		// unfortunately, we're using a global variable to get around the fact that worker processes can't accept arguments
		input_filename = argv[1];
	}
	
	
	int num_processes_with_parent;
	if(sscanf(argv[2], "%d", &num_processes_with_parent) == EOF)
	{
		fprintf(stderr, "Bad input parameter: number of processes with parent = %s. Exiting\n\n", argv[2]);
		exit(-101);
	}
	int num_processes = num_processes_with_parent - 1;

	CarbonStartSim(argc, argv);
	
	reportf("PARENT THREAD: running with %d worker processes\n", num_processes);
	
	
	carbon_thread_t threads[num_processes];
	reportf("PARENT THREAD: DONE initializing Graphite\n");
	
	for(int i = 0; i < num_processes; i++)
	{
		printf("PARENT THREAD: Spawning process: %d\n", i);
		threads[i] = CarbonSpawnThread(worker_main_simple, (void *) i);
	}
	
	for(int i = 0; i < num_processes; i++)
    {
		CarbonJoinThread(threads[i]);
		printf("PARENT THREAD: joining process: %d\n", i);
	}
	
	reportf("\n***PARENT THREAD: Ending par_minisat!\n\n");

	CarbonStopSim();
	return 0;
}
