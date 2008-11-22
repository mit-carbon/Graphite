#include "debug.h"
#include <assert.h>
#include <fstream>
#include <sstream>
using namespace std;

#define MAX_CORE_COUNT (128)
ofstream outfile[(MAX_CORE_COUNT+1)];

VOID debugInit (INT32 tid) {
	stringstream fileName;
	string folder = "output_files/";
	fileName << "Core" << tid << ".txt";
	cerr << fileName.str() << endl;
	outfile[tid].open( (fileName.str()).c_str() );

	assert (outfile[tid].is_open());

	outfile[tid] << "Starting Thread: " << tid << endl;
}

VOID debugFinish (INT32 tid) {
	outfile[tid].close();
}

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
	assert( id < MAX_CORE_COUNT );
	assert( id >= 0 );
	if( id == -1 )
		id = MAX_CORE_COUNT;

	assert (outfile[id].is_open());
	outfile[id] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
//	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < MAX_CORE_COUNT );
	if( id == -1 )
		id = MAX_CORE_COUNT;
	assert (outfile[id].is_open());
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;

}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
//	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < MAX_CORE_COUNT );
	if( id == -1 )
		id = MAX_CORE_COUNT;
	assert (outfile[id].is_open());
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;

}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
//	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
	assert( id < MAX_CORE_COUNT );
	if( id == -1 )
		id = MAX_CORE_COUNT;
	assert (outfile[id].is_open());
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << dec << endl;

}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
//	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < MAX_CORE_COUNT );
	if( id == -1 )
		id = MAX_CORE_COUNT;
	assert (outfile[id].is_open());
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}
