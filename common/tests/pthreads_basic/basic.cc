
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"

using namespace std;

struct cVector
{
		float x,y,z;
};

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
  
  cerr << "Begin Main " << endl;
   int tid;
   CAPI_Initialize(&tid);
  cerr << " After CAPI Initialization " << endl;
   
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
	cerr << "This is the function main()" << endl;
	cerr << "Initializing thread structures" << endl << endl;
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);

#ifdef DEBUG
	cerr << "Spawning threads" << endl << endl;
#endif

      pthread_create(&threads[0], &attr, do_nothing, (void *) 0);    
      pthread_create(&threads[1], &attr, do_nothing, (void *) 1);    

	// Wait for all threads to complete
//	while(true);

#ifdef DEBUG
   pthread_mutex_lock(&lock);
	cerr << "Waiting to join" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


        pthread_join(threads[0], NULL);         
        pthread_join(threads[1], NULL);

#ifdef DEBUG
	cerr << "End of execution" << endl << endl;
#endif
        
   return 0;
} // main ends

void BARRIER_DUAL_CORE(int tid)
{
	//this is a stupid barrier just for the test purposes
	int payload;

	cerr << "BARRIER DUAL CORE for ID(" << tid << ")" << endl;
	if(tid==0) {
		CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
		CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
	} else {
		CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
		CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
	}
}

//spawned threads run this function
void* do_nothing(void *threadid)
{
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "beginning of do_nothing function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
	int tid;
//	cerr << "start capi_init" << endl;
	CAPI_Initialize(&tid);
//	cerr << "end   capi_init" << endl;
#if 0	
   int i = 0;
   int j = 0;
   int k = 0;
	unsigned int cpeinfo;
	unsigned int cpsse3;
   int foo = 10, bar = 15;

	cVector vector;
	vector.x=0.5;
	vector.y=1.5;
	vector.z=-3.141;
/*
asm volatile("addl  %%ebx, %%eax \n\t"
				  "addl  %%ebx, %%eax \n\t"
						 :"=a"(foo)
						 :"a"(foo), "b"(bar)
				 );
*/
if(tid==0) {
//	cerr << endl << "i = " << i << endl;
 	
	asm (
//		"movl %1, %%eax    \n\t"
//		"addl %%eax, %0   \n\t"
//		"movl i, %eax    \n\t"
//		"movl %1, %%eax  "
//		"movl %%eax, %0  "
//		"movl %2, %%eax \n\t;"
//		"movl %%eax, %0 \n\t"
		"movl $1, %%eax \n\t"
		"cpuid \n\t"
		"movl %%edx, %0 \n\t"
		"movl %%ecx, %1 \n\t"
//		"movups %3, %%xmm1 \n\t"
//		"mulps %%xmm1, %%xmm1 \n\t"
//		"movups %%xmm1, %3 \n\t"
		:"=a"(cpeinfo),"=b"(cpsse3)//, "=c"(vector) //output
		:"a"(i)//, "b"(vector)  //input
//		:"%eax"  //clobbered register
	);

//THE INSTRUCTION I WANT IS LDDQU  vector load, unaligned int
//try to 


//		mov eax, 01h       ;01h is the parameter for the CPUID command below
//		cpuid
//		mov cpeinfo, edx   ;Get the info stored by CPUID
//		mov cpsse3, ecx    ;info about SSE3 is stored in ECX
//	cerr << endl << "i = " << i << endl;
	cerr << endl << " cpuid =  " << dec << i << endl;
	cerr << endl << " cpsse3 = " << dec << k << endl;
	
	cerr << "MMX:  " << ((cpeinfo >> 23) & 0x1 ) << "\tSSE:  " << ((cpeinfo >> 25) & 0x1 ) << "\tSSE2: " << ((cpeinfo >> 26) & 0x1 ) << "\n"; 
	cerr << "SSE3: " << ((cpsse3       ) & 0x1 ); 
	cerr << vector.x << " " << vector.y << " " << vector.z << endl;
//
}
else
{
	asm (
	//		"movl %1, %%eax    \n\t"
	//		"addl %%eax, %0   \n\t"
	//		"movl i, %eax    \n\t"
	//		"movl %1, %%eax  "
	//		"movl %%eax, %0  "
		"movl %1, %%eax \n\t;"
		"movl %%eax, %0 \n\t"
		:"=a"(j) //output
		:"a"(j)  //input
	//		:"%eax"  //clobbered register
	);

	cerr << endl << "j = " << j << endl;
}

#endif

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
//   cerr << "executing do_nothing function: " << tid << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
   int size = 10;
   global_integer = 10;
   global_integer_ptr = &global_integer;
   if(tid==0) {
		pthread_mutex_lock(&lock);
		cerr << "Core: " << tid << " being instrumented." << endl;
		cerr << "size addr: " << &size << endl;
		cerr << "gint addr: " << &global_integer << endl;
		cerr << "gint_ptr : " << global_integer_ptr << endl;
		pthread_mutex_unlock(&lock);

		BARRIER_DUAL_CORE(tid);
		instrument_me( );
		
		pthread_mutex_lock(&lock);
		cerr << "Core: " << tid << " finished instrumenting." << endl;
		pthread_mutex_unlock(&lock);
   } else {
		pthread_mutex_lock(&lock);
		cerr << "Core: " << tid << " being instrumented." << endl;
		pthread_mutex_unlock(&lock);

		BARRIER_DUAL_CORE(tid);
		instrument_me( );
		
		pthread_mutex_lock(&lock);
		cerr << "Core: " << tid << " finished instrumenting." << endl;
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
//	cerr << "Greetings I wonder if this will cause any crashes or anything else interesting?1" << endl;
int size = 128;
char array[size];


	//4 bytes first line (at the end), 4 bytes overflow onto second line
	int addr = (((int)array) / 32) * 32 + 30;
	*((UINT64*) addr)  = 0xFFFF;
   UINT64 x = *((UINT64*) addr);

   cerr << hex << "x = " << ((UINT64) x) << endl;

/*
 int size = *global_integer_ptr; 

   cerr << "inside instrument me, size=" << size << endl;
   int array[size];
   int x;

   for(int i=1; i < size-1; i++) {
		array[i] = array[i-1] + i + (int) global_integer_ptr + array[i+1];
   }
   
   for(int i=1; i < size-1; i++) {
		x = array[i];
	}

	for(int i=1; i < size-1; i++) {
		array[i] = x;
	}

	pthread_mutex_lock(&lock);

   cerr << "inside lock " << endl;

   for(int i=1; i < size-1; i++) {
		array[i] = array[i-1] + i + (int) global_integer_ptr + array[i+1];
   }
   
	
   pthread_mutex_unlock(&lock);
*/
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
//   cerr << "   Addr of x: " << hex << &x << endl;
//	cerr << "   Addr of y: " << hex << &y << endl;
//	cerr << "          x = " << x << endl;
	
/* int foo = 10, bar = 15;
 asm volatile("addl  %%ebx,%%eax"
				  "addl  %%ebx, %%eax"
						 :"=a"(foo)
						 :"a"(foo), "b"(bar)
				 );
*/
//	printf("foo+bar=%d\n", foo);

}

