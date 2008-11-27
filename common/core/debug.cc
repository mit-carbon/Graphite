#include "debug.h"
#include <assert.h>
#include <fstream>
#include <sstream>
using namespace std;

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
	pin_thread_count = 2*core_count + 1;
	outfiles = new ofstream[pin_thread_count];

	for(unsigned int i=0; i < pin_thread_count; i++) {
		stringstream fileName;
		string folder = "output_files/";
		fileName << folder << "PinThread_" << i << ".txt";
		outfiles[i].open( (fileName.str()).c_str() );
	}
#endif
}

//close debug logs
VOID debugFinish () {
#ifdef WRITE_FLAG
	for(unsigned int i=0; i < pin_thread_count; i++) {
		outfiles[i].close();
	}
#endif
}

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
#ifdef WRITE_FLAG            
	THREADID pin_tid = PIN_ThreadId();
	assert( pin_tid < pin_thread_count );
	assert( pin_tid >= 0 );
	assert (outfiles[pin_tid].is_open());
	outfiles[pin_tid] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
#endif
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	THREADID pin_tid = PIN_ThreadId();
	assert( pin_tid < pin_thread_count );
	assert( pin_tid >= 0 );
	assert (outfiles[pin_tid].is_open());
	outfiles[pin_tid] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
#ifdef WRITE_FLAG
	THREADID pin_tid = PIN_ThreadId();
	assert( pin_tid < pin_thread_count );
	assert( pin_tid >= 0 );
	assert (outfiles[pin_tid].is_open());
	outfiles[pin_tid] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
#ifdef WRITE_FLAG
	THREADID pin_tid = PIN_ThreadId();
	assert( pin_tid < pin_thread_count );
	assert( pin_tid >= 0 );
	assert (outfiles[pin_tid].is_open());
	outfiles[pin_tid] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
#endif
}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
#ifdef WRITE_FLAG
	THREADID pin_tid = PIN_ThreadId();
	assert( pin_tid < pin_thread_count );
	assert( pin_tid >= 0 );
	assert (outfiles[pin_tid].is_open());
	outfiles[pin_tid] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
#endif
}
