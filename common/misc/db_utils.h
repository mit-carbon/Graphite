#include <db.h>
#include <string>

using std::string;

class DBUtils
{
public:
   static void initialize(DB*& db, const string& db_name, const string& lib_name);
   static int getRecord(DB*& db, DBT& key, DBT& data);
   static int putRecord(DB*& db, DBT& key, DBT& data);

private:
   static void initializeEnv();

   static DB_ENV* _db_env;
};
