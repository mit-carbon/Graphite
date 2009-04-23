#include <elf.h>

VOID copyInitialStackData(CONTEXT *ctxt)
{
   // 1) Command Line Arguments
   // 2) Environment Variables
   // 3) Auxiliary Vector Entries

   // We should also check if we have the main() thread before doing this
   ADDRINT reg_eip = PIN_GetContextReg(ctxt, REG_INST_PTR);
   ADDRINT reg_esp = PIN_GetContextReg(ctxt, REG_STACK_PTR);
   // fprintf (stderr, "Reg_EIP = 0x%x\n", (UInt32) reg_eip);
   // fprintf (stderr, "Reg_ESP = 0x%x\n", (UInt32) reg_esp);

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
      // TODO: Try this out in the test repository first
      assert (sizeof(Elf32_auxv_t) == sizeof(auxiliary_vector[i]));

      core->accessMemory(Core::NONE, WRITE, (ADDRINT) &auxiliary_vector[i], (char*) &auxiliary_vector[i], sizeof(auxiliary_vector[i]));
      if (auxiliary_vector[i].a_type == AT_NULL)
      {
         break;
      }
   }


}

VOID allocateStackSpace()
{
   // Note that 1 core = 1 thread currently
   // We should probably get the amount of stack space per thread from a configuration parameter
   // Each process allocates whatever it is responsible for !!
   // TODO: Fill this in later
   UInt32 stack_size_per_core;

   // TODO: Make sure that this is a multiple of the page size 
   
   // TODO: Fill this in later
   UInt32 total_cores;
   // TODO: We might want to make this configurable per process later. Or we can just start multiple processes in a 16-processor machine
   UInt32 num_cores_per_process;

   // mmap() the total amount of memory at a fixed address - say 0x90000000
   // TODO: Fill this in
   UInt32 global_stack_base = 0x90000000;
   // TODO: Fill this in
   UInt32 curr_process_num = (UInt32) Config::getSingleton()->getCurrentProcessNum();
   
   UInt32 curr_stack_base = global_stack_base + (curr_process_num * num_cores_per_process * stack_size_per_core); 

   assert(mmap((void*) curr_stack_base, stack_size_per_core * num_cores_per_process,  PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) == 0);
   
}
