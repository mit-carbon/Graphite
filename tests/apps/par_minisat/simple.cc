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
Solver S;
Solver* solver;




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class B>
static void skipWhitespace(B& in) {
    while ((*in >= 9 && *in <= 13) || *in == 32)
        ++in; }

template<class B>
static void skipLine(B& in) {
    for (;;){
        if (*in == EOF) return;
        if (*in == '\n') { ++in; return; }
        ++in; } }

template<class B>
static int parseInt(B& in) {
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') fprintf(stderr, "PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
        ++in;
    return neg ? -val : val; }

template<class B>
static void readClause(B& in, Solver& S, vec<Lit>& lits) {
    int     parsed_lit, var;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        while (var >= S.nVars()) S.newVar();
        lits.push( (parsed_lit > 0) ? Lit(var) : ~Lit(var) );
    }
}


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


static void SIGINT_handler(int signum){
    reportf("\n"); reportf("*** INTERRUPTED ***\n");
    printStats(solver->stats);
    reportf("\n"); reportf("*** INTERRUPTED ***\n");
    exit(1);
}


// does the actual parsing work
void parse_DIMACS_main(int /* FIXME */ in, Solver& S) {
#ifdef ORIGINAL
    vec<Lit> lits;
    for (;;){
        skipWhitespace(in);
        if (*in == EOF)
            break;
        else if (*in == 'c' || *in == 'p')
            skipLine(in);
        else
            readClause(in, S, lits);
			S.addClause(lits);

    }
#endif

	// TODO; this is a hacked version
/*	for(;;){
		// TODO: skipwhitespace
		if(*in == EOF)
			break;
		else if (
		}
*/

//	CURRENT STATUS: not able to load file using fstream. "using namespace std" breaks things, so try loading iostream and fstream without it.

	return;
}


void* worker_main_simple(void *threadid) {
	/* Find out my identity */
	long rank = (long)threadid;
	reportf("worker_main_simple[%ld]: Hello from process id %ld\n", rank, rank);

	// TODO: should I be returning something other than NULL here?
	return NULL;
}



void parse_input(int argc, char** argv) {
	// we're only supporting uncompressed file reading. standard CNF format.
	
	if (argc >= 2 && strlen(argv[1]) >= 5 && strcmp(&argv[1][strlen(argv[1])-5], ".bcnf") == 0)
	{
		//parse_BCNF(argv[1], S);
		fprintf(stderr, "ERROR: BNCF format unsupported. exiting...\n");
		exit(-1000);
	}
	else
	{
		if (argc == 1)
		{
            reportf("Reading from STDIN not supported. EXITING. Reading from standard input... Use '-h' or '--help' for help.\n");
			exit(-1001);
		}
		
        // ORIGINAL: gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
		//FILE *in = fopen(argv[1], "rb");

		ifstream in;
		reportf("Attempting to read file %s\n", argv[1]);
		in.open(argv[1], ios::in);

		string line;
		vec<Lit> lits;
		int line_no = 0;
		while (getline(in, line) ) {
			//cout << line << endl;
			
			//skipWhitespace(in);
			//if (*in == 'c' || *in == 'p')
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
						while (var >= S.nVars()) S.newVar();
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

				// psota hack: 
#ifdef DEBUG_PRINT
				printf("START NEW LIT VECTOR____\n");
				for(int i = 0; i < lits.size(); i++) {
					printf("%d.\t%d\n", i, index(lits[i]));
				}
				printf("END NEW LIT VECTOR____\n");
#endif
				
				// psota hack end
				



				S.addClause(lits);

			}
			line_no++;
		}
		in.close();
		
    }
}

int main(int argc, char** argv)
{
	int num_processes;
	
	// help message
	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
	{
		reportf("USAGE: %s <input-file> <result-output-file>\n  where the input may be either in plain/gzipped DIMACS format or in BCNF.\n", argv[0]);
		exit(0);
	}
	
	CarbonStartSim(argc, argv);

	// TODO: turn this into a CLA
	num_processes = 4; // one less than is specified in the Makefile
	
	carbon_thread_t threads[num_processes];
	reportf("PARENT THREAD: DONE initializing Graphite\n");
	reportf("PARENT THREAD: parsing input...\n");
	parse_input(argc, argv);
	reportf("PARENT THREAD: DONE parsing input\n");

	// res is the result output file. for now just assume that the user doesn't specify a result output file
    FILE* res = (argc >= 3) ? fopen(argv[2], "wb") : NULL;

	reportf("PARENT THREAD: Checking for unsatisfiability\n");
    if (!S.okay()){
        if (res != NULL) fprintf(res, "UNSAT\n"), fclose(res);
        reportf("Trivial problem\n");
        reportf("UNSATISFIABLE\n");
        exit(20);
    }
	reportf("PARENT THREAD: DONE: Checking for unsatisfiability\n");


	// TODO: parallelize this portion
	reportf("PARENT THREAD: Solving system\n");
    S.verbosity = 1;
    solver = &S;
    signal(SIGINT,SIGINT_handler);
    signal(SIGHUP,SIGINT_handler);

    S.solve();
	fprintf(stderr, "Done solving system\n");
    printStats(S.stats);
    reportf("\n");
    reportf(S.okay() ? "SATISFIABLE\n" : "UNSATISFIABLE\n");

	reportf("PARENT THREAD: DONE: Solving system\n");

	
	for(int i = 0; i < num_processes; i++)
	{
		printf("Spawning process: %d\n", i);
		threads[i] = CarbonSpawnThread(worker_main_simple, (void *) i);
	}
	
	for(int i = 0; i < num_processes; i++)
    {
		CarbonJoinThread(threads[i]);
	}
	
	reportf("\n***PARENT THREAD: Ending par_minisat!\n\n");
	CarbonStopSim();
	return 0;
}
