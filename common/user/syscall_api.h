#ifndef SYSCALL_API_H
#define SYSCALL_API_H

// This is the dummy function that will get replaced
// by the simulator
extern "C" {
   void runSyscallServer();
}

void initSyscallServer();
void* server_thread(void *dummy);
void quitSyscallServer();


#endif
