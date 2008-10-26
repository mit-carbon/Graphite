
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"

using namespace std;

pthread_mutex_t write_lock;

int global_integer;
int* global_integer_ptr;

#define DEBUG 1

#ifdef DEBUG
pthread_mutex_t lock;
#endif


// Function executed by each thread
void* do_nothing(void *threadid);

//you can set pinsim to ONLY instrument this function
void instrument_me();

int main2(int argc, char* argv[]) {
  
  cout << "Begin Main " << endl;
   int tid;
   CAPI_Initialize(&tid);
  cout << " After CAPI Initialization " << endl;
   
   int size = 1;
   int array[size];

   for(int i=0; i < size; i++) {
      array[i] = i;
   }

   return 0;
}


int main(int argc, char* argv[]){ // main begins

	// Declare threads and related variables
	pthread_t threads[2];
	pthread_attr_t attr;
	
#ifdef DEBUG
	cout << "This is the function main()" << endl;
	cout << "Initializing thread structures" << endl << endl;
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);

#ifdef DEBUG
	cout << "Spawning threads" << endl << endl;
#endif

      pthread_create(&threads[0], &attr, do_nothing, (void *) 0);    
      pthread_create(&threads[1], &attr, do_nothing, (void *) 1);    

#ifdef DEBUG
   pthread_mutex_lock(&lock);
	cout << "Waiting to join" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

	// Wait for all threads to complete
        pthread_join(threads[0], NULL);         
        pthread_join(threads[1], NULL);

#ifdef DEBUG
	cout << "End of execution" << endl << endl;
#endif
        
   return 0;
} // main ends

//spawned threads run this function
void* do_nothing(void *threadid)
{
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cout << "beginning of do_nothing function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
	int tid;
	cout << "start capi_init" << endl;
	CAPI_Initialize(&tid);
	cout << "end   capi_init" << endl;

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cout << "executing do_nothing function: " << tid << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
   int size = 1;
   global_integer = 10;
   global_integer_ptr = &global_integer;
   if(tid==0) {
		pthread_mutex_lock(&lock);
		cout << "Core: " << tid << " being instrumented." << endl;
		cout << "size addr: " << &size << endl;
		cout << "gint addr: " << &global_integer << endl;
		cout << "gint_ptr : " << global_integer_ptr << endl;
		pthread_mutex_unlock(&lock);

		instrument_me( );
		
		pthread_mutex_lock(&lock);
		cout << "Core: " << tid << " finished instrumenting." << endl;
		pthread_mutex_unlock(&lock);
   } else {
		pthread_mutex_lock(&lock);
		cout << "Core: " << tid << " being instrumented." << endl;
		pthread_mutex_unlock(&lock);

//		instrument_me( );
		
		pthread_mutex_lock(&lock);
		cout << "Core: " << tid << " finished instrumenting." << endl;
		pthread_mutex_unlock(&lock);
   }
   CAPI_Finish(tid);
	pthread_exit(NULL);  
}

/*
	asm ( assembler template
		: output operands				(optional)
		: input operands				(optional)
		: list of clobbered registers		(optional)
	);	
*/

//int instrument_me(int tid, int* ptr) 
void instrument_me()
{


 int size = *global_integer_ptr; 

   cout << "inside instrument me, size=" << size << endl;
   int array[size];

   for(int i=1; i < size-1; i++) {
		array[i] = array[i-1] + i + (int) global_integer_ptr + array[i+1];
   }
   
	pthread_mutex_lock(&lock);

   cout << "inside lock " << endl;

   for(int i=1; i < size-1; i++) {
		array[i] = array[i-1] + i + (int) global_integer_ptr + array[i+1];
   }
   
	
   pthread_mutex_unlock(&lock);

//	int x = 10, y;
/*	int x,y;
	
	asm volatile (
			"movl 0x8000000, %0;"
//			"movl %1, %%eax;"
//	asm ("movl %1, %%eax;"
//			"movl %%eax, %0;"
			:"=r"(x)	// y is output operand 
			:"r"(x)	//	x is input operand 
//			:"%eax");//	%eax is clobbered register 
			:);//	%eax is clobbered register 
*/
//   cout << "   Addr of x: " << hex << &x << endl;
//	cout << "   Addr of y: " << hex << &y << endl;
//	cout << "          x = " << x << endl;
	
/* int foo = 10, bar = 15;
 asm volatile("addl  %%ebx,%%eax"
				  "addl  %%ebx, %%eax"
						 :"=a"(foo)
						 :"a"(foo), "b"(bar)
				 );
*/
//	printf("foo+bar=%d\n", foo);

}

