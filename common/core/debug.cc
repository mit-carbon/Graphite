#include "debug.h"
#include <assert.h>
#include <sstream>
using namespace std;

ofstream outfile[2];

VOID debugInit (INT32 tid) {
	stringstream fileName;

	fileName << "Core" << tid << ".txt";
	outfile[tid].open( (fileName.str()).c_str() );
}

VOID debugFinish (INT32 tid) {
	outfile[tid].close();
}

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
	outfile[id] << " [" << id << "] - " << class_name << " - : " << output_string << endl;
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;

}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;

}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;

}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
	assert ( (id == 0) || (id == 1) );
	// cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
	outfile[id] << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}
