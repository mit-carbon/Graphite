#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "access.h"

// Initialize BerkeleyDB Database
void initializeDatabase(DB*& database, string database_filename, string lib_name)
{
   int ret = 0;

   // Get the database abspath
   string tmpdir = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
   database_filename = tmpdir + "/" + database_filename;
   // Create the database handle
   if ((ret = db_create(&database, NULL, 0)) != 0)
   {
      fprintf(stderr, "db_create: %s", db_strerror(ret));
      exit(EXIT_FAILURE);
   }
   
   // Open the database
   if ((ret = database->open(database, NULL, database_filename.c_str(), NULL, DB_HASH, DB_CREATE, 0)) != 0)
   {
      database->err(database, ret, "DB->open");
      exit(EXIT_FAILURE);
   }

   // Check if the version key in database is old, and if yes, truncate the database
   struct stat filestat_buf;
   ret = stat(lib_name.c_str(), &filestat_buf);
   if (ret != 0)
   {
      fprintf(stderr, "Could not stat(%s)\n", lib_name.c_str());
      exit(EXIT_FAILURE);
   }
     
   // Get the last modification time of the DSENT library
   DBT version_key, version_data;
   memset(&version_key, 0, sizeof(version_key));
   memset(&version_data, 0, sizeof(version_data));

   version_key.data = (char*) "VERSION";
   version_key.size = sizeof("VERSION");
     
   ret = database->get(database, NULL, &version_key, &version_data, 0);
   if ((ret == 0) || (ret == DB_NOTFOUND))
   {
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
         database->truncate(database, NULL, NULL, 0);

         memset(&version_data, 0, sizeof(version_data));
         version_data.data = &filestat_buf.st_mtime;
         version_data.size = sizeof(filestat_buf.st_mtime);
         // Insert version number into the database
         ret = database->put(database, NULL, &version_key, &version_data, DB_NOOVERWRITE);
         if ((ret != 0) && (ret != DB_KEYEXIST))
         {
            database->err(database, ret, "DB->put");
            exit(EXIT_FAILURE);
         }
      }
   }
   else // ((ret != 0) && (ret != DB_NOTFOUND))
   {
      database->err(database, ret, "DB->get");
      exit(EXIT_FAILURE);
   }
}
