#include "debug.h"

VOID debugPrint(INT32 id, string class_name, string output_string) 
{
	cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
}

// frankly, just use stringstream instead of the functions below
VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
	cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
	cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
	cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
	cerr << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}
