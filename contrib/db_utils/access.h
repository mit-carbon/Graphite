#pragma once

#include <string>
#include <db.h>

using std::string;

// Initialize BerkeleyDB Database
void initializeDatabase(DB*& database, string database_filename, string lib_name);
