#include "debug.h"


VOID debugPrint(INT32 id, string class_name, string output_string) 
{
	cout << "   [" << id << "]  - " << class_name << " - : " << output_string << endl;
}

VOID debugPrint(INT32 id, string class_name, string output_string, int value) 
{
	cout << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}

VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value) 
{
	cout << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}

VOID debugPrintHex(INT32 id, string class_name, string output_string, int value) 
{
	cout << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << hex << value << endl;
}

VOID debugPrintString(INT32 id, string class_name, string output_string, string value) 
{
	cout << "   [" << id << "]  - " << class_name << " - : " << output_string << " = " << value << endl;
}
