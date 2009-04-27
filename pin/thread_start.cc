#include "thread_start.h"
#include "log.h"
#include "core.h"
#include "simulator.h"
#include "fixed_types.h"
#include "pin_config.h"
#include "core_manager.h"
#include "thread_manager.h"
#include "thread_support.h"

#include <sys/mman.h>

int spawnThreadSpawner(CONTEXT *ctxt)
{
   int res;

   ADDRINT reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);

   PIN_LockClient();
   
   AFUNPTR thread_spawner;
   IMG img = IMG_FindByAddress(reg_eip);
   RTN rtn = RTN_FindByName(img, "CarbonSpawnThreadSpawner");
   thread_spawner = RTN_Funptr(rtn);

   PIN_UnlockClient();
   
   PIN_CallApplicationFunction(ctxt,
            PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            thread_spawner,
            PIN_PARG(int), &res,
            PIN_PARG_END());
      
   LOG_ASSERT_ERROR(res == 0, "Failed to spawn Thread Spawner");

   return res;
}

VOID copyStaticData(IMG& img)
{
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   LOG_ASSERT_ERROR (core != NULL, "Does not have a valid Core ID");

   for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
   {
      ADDRINT sec_address;

      // I am not sure whether we want ot copy over all the sections or just the
      // sections which are relevant like the sections below: DATA, BSS, GOT
      
      // Copy over all the sections now !
      // SEC_TYPE sec_type = SEC_Type(sec);
      // if ( (sec_type == SEC_TYPE_DATA) || (sec_type == SEC_TYPE_BSS) ||
      //     (sec_type == SEC_TYPE_GOT)
      //   )
      {
         if (SEC_Mapped(sec))
         {
            sec_address = SEC_Address(sec);
         }
         else
         {
            sec_address = (ADDRINT) SEC_Data(sec);
         }

         LOG_PRINT ("Copying Section: %s at Address: 0x%x of Size: %u to Simulated Memory", 
               SEC_Name(sec).c_str(), (UInt32) sec_address, (UInt32) SEC_Size(sec));
         core->accessMemory(Core::NONE, WRITE, sec_address, (char*) sec_address, SEC_Size(sec));
      }
   }
}

VOID copyInitialStackData(ADDRINT reg_esp)
{
   Core* core = Sim()->getCoreManager()->getCurrentCore();
   LOG_ASSERT_ERROR (core != NULL, "Does not have a valid Core ID");

   // 1) Command Line Arguments
   // 2) Environment Variables
   // 3) Auxiliary Vector Entries

   ADDRINT params = reg_esp;
   int argc = * ((int *) params);
   char **argv = (char **) (params + sizeof(int));
   char **envir = argv+argc+1;

   Elf32_auxv_t* auxiliary_vector;

   // fprintf (stderr, "argc = %d\n", argc);
   // Write argc
   core->accessMemory(Core::NONE, WRITE, params, (char*) &argc, sizeof(argc));

   fprintf (stderr, "Copying Command Line Arguments to Simulated Memory\n");
   for (SInt32 i = 0; i < argc; i++)
   {
      // Writing argv[i]
      // fprintf (stderr, "argv[%d] = %s\n", i, argv[i]);
      core->accessMemory(Core::NONE, WRITE, (ADDRINT) &argv[i], (char*) &argv[i], sizeof(argv[i]));
      core->accessMemory(Core::NONE, WRITE, (ADDRINT) argv[i], (char*) argv[i], strlen(argv[i])+1);
   }

   // I dont know what this is but it might be worth copying it over
   // I have found this to be '0' in most cases
   core->accessMemory(Core::NONE, WRITE, (ADDRINT) &argv[argc], (char*) &argv[argc], sizeof(argv[argc]));

   fprintf (stderr, "Copying Environmental Variables to Simulated Memory\n");
   // We need to copy over the environmental parameters also
   for (SInt32 i = 0; ; i++)
   {
      // Writing environ[i]
      // fprintf (stderr, "envir[%d] = %s\n", i, envir[i]);
      if (envir[i] == 0)
      {
         core->accessMemory(Core::NONE, WRITE, (ADDRINT) &envir[i], (char*) &envir[i], sizeof(envir[i]));
         auxiliary_vector = (Elf32_auxv_t*) &envir[i+1]; 
         break;
      }
      else
      {
         core->accessMemory(Core::NONE, WRITE, (ADDRINT) &envir[i], (char*) &envir[i], sizeof(envir[i]));
         core->accessMemory(Core::NONE, WRITE, (ADDRINT) envir[i], (char*) envir[i], strlen(envir[i])+1);
      }
   }

   for (SInt32 i = 0; ; i++)
   {
      // We should have a per auxiliary vector entry based handling
      // Right now, just copy stuff over
      assert (sizeof(Elf32_auxv_t) == sizeof(auxiliary_vector[i]));

      core->accessMemory(Core::NONE, WRITE, (ADDRINT) &auxiliary_vector[i], (char*) &auxiliary_vector[i], sizeof(auxiliary_vector[i]));
      if (auxiliary_vector[i].a_type == AT_NULL)
      {
         break;
      }
   }

}

VOID copySpawnedThreadStackData(ADDRINT reg_esp)
{
   core_id_t core_id = PinConfig::getSingleton()->getCoreIDFromStackPtr(reg_esp);

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreID(core_id, stack_attr);

   ADDRINT stack_upper_limit = stack_attr.lower_limit + stack_attr.size;
   
   UInt32 num_bytes_to_copy = (UInt32) (stack_upper_limit - reg_esp);

   Core* core = Sim()->getCoreManager()->getCurrentCore();

   core->accessMemory(Core::NONE, WRITE, reg_esp, (char*) reg_esp, num_bytes_to_copy);

}

VOID allocateStackSpace()
{
   // Note that 1 core = 1 thread currently
   // We should probably get the amount of stack space per thread from a configuration parameter
   // Each process allocates whatever it is responsible for !!
   UInt32 stack_size_per_core = PinConfig::getSingleton()->getStackSizePerCore();
   UInt32 num_cores = Sim()->getConfig()->getNumLocalCores();
   UInt32 stack_base = PinConfig::getSingleton()->getStackLowerLimit();

   // TODO: Make sure that this is a multiple of the page size 
   
   // mmap() the total amount of memory needed for the stacks
   assert(mmap((void*) stack_base, stack_size_per_core * num_cores,  PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) == 0);
  
   // TODO: From our memory manager, mark this space as taken - Implement mmap() and brk()
}

VOID SimPthreadAttrInitOtherAttr(pthread_attr_t *attr)
{
   core_id_t core_id;
   
   ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();
   core_id = req->core_id;

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreID(core_id, stack_attr);

   pthread_attr_setstack(attr, (void*) stack_attr.lower_limit, stack_attr.size);
}
