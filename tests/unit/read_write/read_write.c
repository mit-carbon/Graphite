#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "carbon_user.h"

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   char* read_file = "./tests/unit/read_write/read_file";
   char* write_file = "./tests/unit/read_write/write_file";
   char* str1[2] = {"read1\n", "read2\n"};
   char* str2[2] = {"write1\n", "write2\n"};
   char read_buf[10];

   // Read data
   FILE* fp_read = fopen(read_file, "r");
   int i = 0;
   while (fgets(read_buf, 10, fp_read) != NULL)
   {
      printf("%s", read_buf);
      if (i > 1)
      {
         fprintf(stderr, "TEST FAILED, Read more than 2 lines\n");
         exit(-1);
      }

      if (strncmp(str1[i], read_buf, 10) != 0)
      {
         fprintf(stderr, "TEST FAILED, values dont match: read_buf(%s), str1[%i](%s)\n", read_buf, i, str1[i]);
         exit(-1);
      }
      i++;
   }
   assert(fclose(fp_read) == 0);

   // Write data
   FILE* fp_write = fopen(write_file, "w");
   assert(fputs(str2[0], fp_write) > 0);
   assert(fputs(str2[1], fp_write) > 0);
   assert(fclose(fp_write) == 0);

   // Read and verify the data that was written
   fp_write = fopen(write_file, "r");
   i = 0;
   while (fgets(read_buf, 10, fp_write) != NULL)
   {
      printf("%s", read_buf);
      if (i > 1)
      {
         fprintf(stderr, "TEST FAILED, Read more than 2 lines\n");
         exit(-1);
      }

      if (strncmp(str2[i], read_buf, 10) != 0)
      {
         fprintf(stderr, "TEST FAILED, values dont match: read_buf(%s), str2[%i](%s)\n", read_buf, i, str2[i]);
         exit(-1);
      }
      i++;
   }
   assert(fclose(fp_write) == 0);

   printf("Read-Write TEST SUCCESS\n");

   CarbonStopSim();
   return 0;
}
