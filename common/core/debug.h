#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <iostream>
#include <fstream>
#include "pin.H"

using namespace std;

	//id is signed, because we may want to pass "-1" to denote we don't know the id
	VOID debugInit(INT32 tid);
	VOID debugFinish (INT32 tid);
	VOID debugPrint(INT32 id, string class_name, string output_string);
  
	VOID debugPrint(INT32 id, string class_name, string output_string, int value);
	VOID debugPrintHex(INT32 id, string class_name, string output_string, int value);
	VOID debugPrintFloat(INT32 id, string class_name, string output_string, float value);
	VOID debugPrintString(INT32 id, string class_name, string output_string, string value);

#endif
