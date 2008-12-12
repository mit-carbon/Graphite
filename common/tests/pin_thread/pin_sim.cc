
#include <iostream>
#include <syscall.h>
#include "pin.H"
#include "thread_test.h"
#include <cassert>


using namespace std;

void SIMcarbonStartShthreads()
{
   cerr << "Starting the threads." << endl;
   OS_SERVICES::ITHREAD *my_thread_p;
   TestThread my_thread_runner = TestThread();
   my_thread_p = OS_SERVICES::ITHREADS::GetSingleton()->Spawn(4096, &my_thread_runner);
   assert(my_thread_p);
    //cerr << "my_thread_p: " << hex << my_thread_p << endl;
   //cerr << "is thread active?: " << my_thread_p->IsActive() << endl;;
//   my_thread_runner.RunThread(my_thread_p);
   cout << "===================================" << endl;
   cout << "And that thread has been started..." << endl;
   cout << "===================================" << endl;
   sleep(5);
}


int main(int argc, char *argv[])
{
    PIN_Init(argc, argv);

    SIMcarbonStartShthreads();

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

