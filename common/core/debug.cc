#include "debug.h"
#include <assert.h>
#include <fstream>
#include <sstream>
using namespace std;

//turn this flag off to prevent creating and writing log files
#define WRITE_FLAG
ofstream* outfiles;
UINT32 core_count;

//open debug logs
VOID debugInit (UINT32 core_count_arg) {
	//FIXME will this work when we go across clusters?
#ifdef WRITE_FLAG
	core_count = core_count_arg;
	outfiles = new ofstream[core_count];

	for(unsigned int i=0; i < core_count; i++) {
		stringstream fileName;
		string folder = "output_files/";
		fileName << folder << "Core" << i << ".txt";
		outfiles[i].open( (fileName.str()).c_str() );
	}
#endif
}

//close debug logs
VOID debugFinish () {
#ifdef WRITE_FLAG
	for(unsigned int i=0; i < core_count; i++) {
		outfiles[i].close();
	}
#endif
}

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
	//TODO handle "-1" ids or other ids that may want to write
	assert( id < (INT32) core_count );
	assert (outfile[id].is_open());
	if( id >= 0 ) 
		outfiles[id] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
#endif
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < (INT32) core_count );
	assert (outfile[id].is_open());
	if( id >= 0 ) 
		outfiles[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
#ifdef WRITE_FLAG
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	assert( id < (INT32) core_count );
	assert (outfile[id].is_open());
	if( id >= 0)
		outfiles[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	assert( id < (INT32) core_count );
	assert (outfile[id].is_open());
	if( id >= 0)
		outfiles[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
#endif
}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
#ifdef WRITE_FLAG
	assert( id < (INT32) core_count );
	assert (outfile[id].is_open());
	if( id >= 0)
		outfiles[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}
