#include <cstdlib>
#include "api.h"

namespace DBUtils
{

int getRecord(DB*& db, DBT& key, DBT& data)
{
   int ret = db->get(db, NULL, &key, &data, 0);
   if ((ret != 0) && (ret != DB_NOTFOUND))
   {
      db->err(db, ret, "DB->get");
      exit(EXIT_FAILURE);
   }
   return ret;
}
   
int putRecord(DB*& db, DBT& key, DBT& data)
{
   int ret;

   // Insert version number into the database
   ret = db->put(db, NULL, &key, &data, DB_NOOVERWRITE);
   if ((ret != 0) && (ret != DB_KEYEXIST))
   {
      db->err(db, ret, "DB->put");
      exit(EXIT_FAILURE);
   }

   // Sync to disk
   if ((ret = db->sync(db, 0)) != 0)
   {
      db->err(db, ret, "DB->sync");
      exit(EXIT_FAILURE);
   }

   return ret;
}

}
