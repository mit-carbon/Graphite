#include <db.h>
#include <string>

using std::string;

namespace DBUtils
{

void initialize(DB*& db, const string& db_name, const string& lib_name);
int getRecord(DB*& db, DBT& key, DBT& data);
int putRecord(DB*& db, DBT& key, DBT& data);

}
