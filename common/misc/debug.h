#ifndef DEBUG_H
#define DEBUG_H

#include <string>
#include <iostream>
#include <fstream>
#include "fixed_types.h"

using namespace std;

	//id is signed, because we may want to pass "-1" to denote we don't know the id
	void debugInit(UInt32 core_count);
	void debugFinish ();
	void debugPrintStart(SInt32 id, string class_name, string output_string);
	void debugPrint(SInt32 id, string class_name, string output_string);
  
	void debugPrint(SInt32 id, string class_name, string output_string, int value);
	void debugPrintHex(SInt32 id, string class_name, string output_string, int value);
	void debugPrintFloat(SInt32 id, string class_name, string output_string, float value);
	void debugPrintString(SInt32 id, string class_name, string output_string, string value);

#endif
