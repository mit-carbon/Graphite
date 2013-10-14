#include <string.h>
#include <sys/mman.h>

#include "thread_start.h"
#include "log.h"
#include "core.h"
#include "simulator.h"
#include "fixed_types.h"
#include "pin_config.h"
#include "tile_manager.h"
#include "thread_manager.h"
#include "thread_support.h"

int spawnThreadSpawner(CONTEXT *ctxt)
{
   int res;

   IntPtr reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);

   PIN_LockClient();
   
   AFUNPTR thread_spawner;
   IMG img = IMG_FindByAddress(reg_eip);
   RTN rtn = RTN_FindByName(img, "CarbonSpawnThreadSpawner");
   thread_spawner = RTN_Funptr(rtn);

   PIN_UnlockClient();
   LOG_ASSERT_ERROR( thread_spawner != NULL, "ThreadSpawner function is null. You may not have linked to the carbon APIs correctly.");
   
   LOG_PRINT("Starting CarbonSpawnThreadSpawner(%p)", thread_spawner);
   
   PIN_CallApplicationFunction(ctxt,
            PIN_ThreadId(),
            CALLINGSTD_DEFAULT,
            thread_spawner,
            PIN_PARG(int), &res,
            PIN_PARG_END());

   LOG_PRINT("Thread spawner spawned");
   LOG_ASSERT_ERROR(res == 0, "Failed to spawn Thread Spawner");

   return res;
}

VOID copyStaticData(IMG& img)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   LOG_ASSERT_ERROR (core != NULL, "Does not have a valid Core ID");

   for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec))
   {
      IntPtr sec_address;

      // I am not sure whether we want ot copy over all the sections or just the
      // sections which are relevant like the sections below: DATA, BSS, GOT
      
      // Copy all the mapped sections except the executable section now
      SEC_TYPE sec_type = SEC_Type(sec);
      if (sec_type != SEC_TYPE_EXEC)
      {
         if (SEC_Mapped(sec))
         {
            sec_address = SEC_Address(sec);

            LOG_PRINT ("Copying Section: %s at Address: 0x%x of Size: %u to Simulated Memory", SEC_Name(sec).c_str(), (UInt32) sec_address, (UInt32) SEC_Size(sec));
            core->accessMemory(Core::NONE, Core::WRITE, sec_address, (char*) sec_address, SEC_Size(sec));
         }
      }
   }
}

VOID copyInitialStackData(IntPtr& reg_esp, core_id_t core_id)
{
   // We should not get core_id for this stack_ptr
   Core* core = Sim()->getTileManager()->getCurrentCore();
   LOG_ASSERT_ERROR (core != NULL, "Does not have a valid Core ID");

   // 1) Command Line Arguments
   // 2) Environment Variables
   // 3) Auxiliary Vector Entries

   SInt32 initial_stack_size = 0;

   IntPtr stack_ptr_base;
   IntPtr stack_ptr_top;
   
   IntPtr params = reg_esp;

   carbon_reg_t argc = * ((carbon_reg_t *) params);
   char **argv = (char **) (params + sizeof(carbon_reg_t));
   char **envir = argv+argc+1;

   //////////////////////////////////////////////////////////////////////
   // Pass 1
   // Determine the Initial Stack Size
   // Variables: size_argv_ptrs, size_env_ptrs, size_information_block 
   //////////////////////////////////////////////////////////////////////

   // Write argc
   initial_stack_size += sizeof(argc);

   // Write argv
   for (SInt32 i = 0; i < (SInt32) argc; i++)
   {
      // Writing argv[i]
      initial_stack_size += sizeof(char*);
      initial_stack_size += (strlen(argv[i]) + 1);
   }

   // A '0' at the end 
   initial_stack_size += sizeof(char*);

   // We need to copy over the environmental parameters also
   for (SInt32 i = 0; ; i++)
   {
      // Writing environ[i]
      initial_stack_size += sizeof(char*);
      if (envir[i] == 0)
      {
         break;
      }
      initial_stack_size += (strlen(envir[i]) + 1);
   }

   // Auxiliary Vector Entry
   initial_stack_size += sizeof(Elf64_auxv_t);


   //////////////////////////////////////////////////////////////////////
   // Pass 2
   // Copy over the actual data
   // Variables: stack_ptr_base_sim
   //////////////////////////////////////////////////////////////////////
   

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreAndThreadID (core_id, 0, stack_attr);
   stack_ptr_top = stack_attr.lower_limit + stack_attr.size;
   stack_ptr_base = stack_ptr_top - initial_stack_size;
   stack_ptr_base = (stack_ptr_base >> (sizeof(IntPtr))) << (sizeof(IntPtr));
   
   // Assign the new ESP
   reg_esp = stack_ptr_base;

   // fprintf (stderr, "argc = %d\n", argc);
   // Write argc
   core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &argc, sizeof(argc));
   stack_ptr_base += sizeof(argc);

   LOG_PRINT("Copying Command Line Arguments to Simulated Memory");
   LOG_PRINT("From 0x%x to 0x%x", reg_esp, stack_ptr_base);
   for (SInt32 i = 0; i < (SInt32) argc; i++)
   {
      // Writing argv[i]
      stack_ptr_top -= (strlen(argv[i]) + 1);
      core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_top, (char*) argv[i], strlen(argv[i])+1);

      core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &stack_ptr_top, sizeof(stack_ptr_top));
      stack_ptr_base += sizeof(stack_ptr_top);
   }

   // I have found this to be '0' in most cases
   core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &argv[argc], sizeof(argv[argc]));
   stack_ptr_base += sizeof(argv[argc]);

   // We need to copy over the environmental parameters also
   LOG_PRINT("Copying Environmental Variables to Simulated Memory");
   for (SInt32 i = 0; ; i++)
   {
      // Writing environ[i]
      if (envir[i] == 0)
      {
         core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &envir[i], sizeof(envir[i]));
         stack_ptr_base += sizeof(envir[i]);
         break;
      }

      stack_ptr_top -= (strlen(envir[i]) + 1);
      core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_top, (char*) envir[i], strlen(envir[i])+1);

      core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &stack_ptr_top, sizeof(stack_ptr_top));
      stack_ptr_base += sizeof(stack_ptr_top);
   }
   

   LOG_PRINT("Copying Auxiliary Vector to Simulated Memory");
  
   Elf64_auxv_t auxiliary_vector_entry_null;

   auxiliary_vector_entry_null.a_type = AT_NULL;
   auxiliary_vector_entry_null.a_un.a_val = 0;

   core->accessMemory(Core::NONE, Core::WRITE, stack_ptr_base, (char*) &auxiliary_vector_entry_null, sizeof(auxiliary_vector_entry_null));
   stack_ptr_base += sizeof(auxiliary_vector_entry_null);

   LOG_ASSERT_ERROR(stack_ptr_base <= stack_ptr_top, "stack_ptr_base = 0x%x, stack_ptr_top = 0x%x", stack_ptr_base, stack_ptr_top);

}

VOID copySpawnedThreadStackData(IntPtr reg_esp)
{
   core_id_t core_id = PinConfig::getSingleton()->getCoreIDFromStackPtr(reg_esp);
   thread_id_t thread_idx = PinConfig::getSingleton()->getThreadIDFromStackPtr(reg_esp);

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreAndThreadID(core_id, thread_idx, stack_attr);

   IntPtr stack_upper_limit = stack_attr.lower_limit + stack_attr.size;
   
   UInt32 num_bytes_to_copy = (UInt32) (stack_upper_limit - reg_esp);

   Core* core = Sim()->getTileManager()->getCurrentCore();

   core->accessMemory(Core::NONE, Core::WRITE, reg_esp, (char*) reg_esp, num_bytes_to_copy);

}

VOID allocateStackSpace()
{
   // Note that 1 core = 1 thread currently
   // We should probably get the amount of stack space per thread from a configuration parameter
   // Each process allocates whatever it is responsible for !!
   __attribute__((unused)) UInt32 stack_size_per_core = PinConfig::getSingleton()->getStackSizePerCore();
   __attribute__((unused)) UInt32 num_tiles = Sim()->getConfig()->getNumLocalTiles();
   __attribute__((unused)) UInt32 max_threads_per_core = PinConfig::getSingleton()->getMaxThreadsPerCore();
   __attribute__((unused)) IntPtr stack_base = PinConfig::getSingleton()->getStackLowerLimit();

   LOG_PRINT("allocateStackSpace: stack_size_per_core = 0x%x", stack_size_per_core);
   LOG_PRINT("allocateStackSpace: num_local_cores = %i", num_tiles);
   LOG_PRINT("allocateStackSpace: max_threads_per_core = %i", max_threads_per_core);
   LOG_PRINT("allocateStackSpace: stack_base = 0x%x", stack_base);

   // TODO: Make sure that this is a multiple of the page size 
   
   // mmap() the total amount of memory needed for the stacks
   __attribute__((unused)) void* mmrtn = mmap((void*) stack_base, stack_size_per_core * num_tiles,  PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
   
   LOG_ASSERT_ERROR((mmrtn == (void*) stack_base),
         "mmap(%p, %u) failed: Cannot allocate stack on host machine", (void*) stack_base, stack_size_per_core * num_tiles);
}

VOID SimPthreadAttrInitOtherAttr(pthread_attr_t *attr)
{
   LOG_PRINT ("In SimPthreadAttrInitOtherAttr");

   //tile_id_t tile_id;
   core_id_t core_id;
   thread_id_t thread_idx;
   
   ThreadSpawnRequest* req = Sim()->getThreadManager()->getThreadSpawnReq();

   if (req == NULL)
   {
      // This is the thread spawner
      core_id = Sim()->getConfig()->getCurrentThreadSpawnerCoreId();
      thread_idx = 0;
   }
   else
   {
      // This is an application thread
      core_id = (core_id_t) {req->destination.tile_id, req->destination.core_type};
      thread_idx = req->destination_tidx;
   }

   PinConfig::StackAttributes stack_attr;
   PinConfig::getSingleton()->getStackAttributesFromCoreAndThreadID(core_id, thread_idx, stack_attr);

   pthread_attr_setstack(attr, (void*) stack_attr.lower_limit, stack_attr.size);

   LOG_PRINT ("Done with SimPthreadAttrInitOtherAttr");
}
