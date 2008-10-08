
#include <iostream>
#include <syscall.h>
#include "pin.H"

bool called_open = false;

using namespace std;

void SyscallEntry(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
   int syscall_number = PIN_GetSyscallNumber(ctxt, std);

   switch(syscall_number)
   {
      case SYS_open:
      {
         char *path = (char *)PIN_GetSyscallArgument(ctxt, std, 0);
         if(!strcmp(path,"./input"))
         {
            called_open = true;
            cout << "open(" << path << ")" << endl;

	    // safer than letting the original syscall go
	    PIN_SetSyscallNumber(ctxt, std, SYS_getpid);

#if 0
            int return_addr = PIN_GetContextReg(ctxt, REG_INST_PTR);
            return_addr += 2;
            PIN_SetContextReg(ctxt, REG_INST_PTR, return_addr);
            PIN_SetContextReg(ctxt, REG_EAX, 0x8);

            PIN_ExecuteAt(ctxt);
#endif
         }

         break;
      }
      default:
         break;
   }
}

void SyscallExit(THREADID threadIndex, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v)
{
   if(called_open)
   {
      called_open = false;
      PIN_SetContextReg(ctxt, REG_EAX, 0x7);
   }
}


int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    PIN_AddSyscallEntryFunction(SyscallEntry, 0);
    PIN_AddSyscallExitFunction(SyscallExit, 0);


    // Never returns
    PIN_StartProgram();
    
    return 0;
}

