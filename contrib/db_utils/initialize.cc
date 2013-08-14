#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "api.h"

namespace DBUtils
{

static DB_ENV* _db_env = NULL;

void initializeEnv()
{
   if (_db_env == NULL)
   {
      string tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
   
      int ret;
      // Create the database env handle
      if ((ret = db_env_create(&_db_env, 0)) != 0)
      {
         fprintf(stderr, "db_env_create: %s\n", db_strerror(ret));
         exit(EXIT_FAILURE);
      }
     
      // Open the database env
      string dbenv_directory = tmpdir + "/dbenv-" + (string) getenv("USER");
      string mkdir_dbenv = "mkdir -p " + dbenv_directory;
      if ((ret = system(mkdir_dbenv.c_str())) != 0)
      {
         fprintf(stderr, "Failed to create %s directory\n", dbenv_directory.c_str());
         exit(EXIT_FAILURE);
      }

      if ((ret = _db_env->open(_db_env, dbenv_directory.c_str(), DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL, 0)) != 0)
      {
         _db_env->err(_db_env, ret, "DB_ENV->open");
         exit(EXIT_FAILURE);
      }
   }
}

void initialize(DB*& db, const string& db_name, const string& lib_name)
{
   initializeEnv();

   string tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
   
   // Get the database file abspath
   string db_filename = tmpdir + "/" + db_name + "-" + (string) getenv("USER") + ".db";

   int ret;
   // Create the database handle
   if ((ret = db_create(&db, _db_env, 0)) != 0)
   {
      fprintf(stderr, "db_create: %s", db_strerror(ret));
      exit(EXIT_FAILURE);
   }
   
   // Open the database
   if ((ret = db->open(db, NULL, db_filename.c_str(), NULL, DB_HASH, DB_CREATE, 0)) != 0)
   {
      db->err(db, ret, "DB->open");
      exit(EXIT_FAILURE);
   }

   // Check if the version key in database is old, and if yes, truncate the database
   struct stat filestat_buf;
   ret = stat(lib_name.c_str(), &filestat_buf);
   if (ret != 0)
   {
      fprintf(stderr, "stat(%s) returns %i\n", lib_name.c_str(), ret);
      exit(EXIT_FAILURE);
   }
     
   // Get the last modification time of the DSENT library
   DBT version_key, version_data;
   memset(&version_key, 0, sizeof(version_key));
   memset(&version_data, 0, sizeof(version_data));

   version_key.data = (char*) "VERSION";
   version_key.size = sizeof("VERSION");
     
   ret = getRecord(db, version_key, version_data);
   
   if ( (ret == 0) &&
        (version_data.size == sizeof(filestat_buf.st_mtime)) && 
        (memcmp(version_data.data, &filestat_buf.st_mtime, version_data.size) == 0)
      )
   {
      // Versions are the same, DO NOT do anything
   }
   else
   {
      // Truncate (/remove all entries) in database
      db->truncate(db, NULL, NULL, 0);

      memset(&version_data, 0, sizeof(version_data));
      version_data.data = &filestat_buf.st_mtime;
      version_data.size = sizeof(filestat_buf.st_mtime);
      // Insert version number into the database
      putRecord(db, version_key, version_data);
   }
}

}
