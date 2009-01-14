#include "debug.h"
#include <assert.h>
#include <fstream>
#include <sstream>
using namespace std;

INT32 chipRank(INT32 *);

//turn this flag off to prevent creating and writing log files
#define WRITE_FLAG
ofstream* outfiles;
UINT32 core_count;
UINT32 pin_thread_count;

//open debug logs
VOID debugInit (UINT32 core_count_arg) {
	//FIXME will this work when we go across clusters?
#ifdef WRITE_FLAG
	core_count = core_count_arg;
	outfiles = new ofstream[2 * core_count];

	cerr << "Number of Cores Simulated (including MCP) = " << core_count << endl;
	cerr << "Number of threads (including shared memory threads) = " << 2 * core_count << endl;

	for(unsigned int i = 0; i < 2 * core_count; i++) {
		stringstream fileName;
		string folder = "output_files/";
		fileName << folder << "Core" << i << ".txt";
		cerr << "Creating File: " << fileName.str() << endl;
		outfiles[i].open( (fileName.str()).c_str() );
		assert (outfiles[i].is_open());
		outfiles[i] << "File: " << i << endl;
	}
#endif
}

//close debug logs
VOID debugFinish () {
#ifdef WRITE_FLAG
	for(unsigned int i = 0; i < 2 * core_count; i++) {
		outfiles[i].close();
	}
#endif
}

VOID debugPrintStart (INT32 id, string class_name, string output_string) {

#ifdef WRITE_FLAG
	assert (id < (INT32) core_count);

	outfiles[id] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
#endif
}

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
	//TODO handle "-1" ids or other ids that may want to write
	assert( id < (INT32) core_count );

	int rank;
	int fileId;

	chipRank (&rank);	

	if (rank == -1) {
		fileId = id + core_count;
	}
	else {
		assert (rank == id);
		fileId = id;
	}
	
	assert ( (fileId >= 0)  &&  (fileId < (2 * (int) core_count)) );
	outfiles[fileId] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
#endif
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < (INT32) core_count );

	int rank;
	int fileId;

	chipRank (&rank);	

	if (rank == -1) {
		fileId = id + core_count;
	}
	else {
		assert (rank == id);
		fileId = id;
	}
	
	assert ( (fileId >= 0)  &&  (fileId < (2 * (int) core_count)) );
	
		 
	outfiles[fileId] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < (INT32) core_count );

	int rank;
	int fileId;

	chipRank (&rank);	

	if (rank == -1) {
		fileId = id + core_count;
	}
	else {
		assert (rank == id);
		fileId = id;
	}
	
	assert ( (fileId >= 0)  &&  (fileId < (2 * (int) core_count)) );
	
		 
	outfiles[fileId] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	assert( id < (INT32) core_count );

	int rank;
	int fileId;

	chipRank (&rank);	

	if (rank == -1) {
		fileId = id + core_count;
	}
	else {
		assert (rank == id);
		fileId = id;
	}
	
	assert ( (fileId >= 0)  &&  (fileId < (2 * (int) core_count)) );
	
		 
	outfiles[fileId] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << dec << endl;
#endif
}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
#ifdef WRITE_FLAG
	assert( id < (INT32) core_count );

	int rank;
	int fileId;

	chipRank (&rank);	

	if (rank == -1) {
		fileId = id + core_count;
	}
	else {
		assert (rank == id);
		fileId = id;
	}
	
	assert ( (fileId >= 0)  &&  (fileId < (2 * (int) core_count)) );
	
		 
	outfiles[fileId] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}
