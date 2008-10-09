// Jonathan Eastep, Charles Gruenwald, Jason Miller
//
// FIXME: this is a hack. 
//
// Includes temporary hooks for the syscall server. Otherwise, this is
// the syscall server. The runSyscallServer hook function is to be 
// manually inserted in main thread of the user app. It calls the code
// the implements the real server. Putting the hook in the user code's 
// main thread gives the server a place to run; we avoid having to spawn 
// a thread or something in the simulator to house the server code.  

#ifndef SYSCALL_SERVER_H
#define SYSCALL_SERVER_H

#include "transport.h"
#include <iostream>

using namespace std;


// externed so the names don't get name-mangled
extern "C" {
   
   void initSyscallServer();
   void runSyscallServer();
   void finiSyscallServer();

}

class SyscallServer {
   private:
      Transport pt_endpt;

   public:
      SyscallServer();

      // interfaces with queues in PT layer to carry out syscalls 
      void run();

};



#endif
