// Harshad Kasture
//

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <sched.h>
#include <syscall.h>
#include <linux/futex.h>
#include <linux/kernel.h>
#include <unistd.h>
#include <errno.h>
#include <queue>
#include "pin.H"
#include "config.h"


class Transport{
   private:
      int pt_tid;
      int pt_num_mod;

      //***** Data structures for normal communication *****//
      typedef struct PTQueue
      {
         queue <char*, deque<char*> > pt_queue;
	 PIN_LOCK pt_q_lock;
      } PTQueue;

      static PTQueue *pt_queue;

      typedef struct Futex
      {
         int futx;
	 PIN_LOCK futx_lock;
      } Futex;

      static Futex *pt_futx;

      //***** Data structures for MCP communication *****//
      //typedef pair<UInt8*, UInt32> MCPEntry;
      struct MCPEntry
      {
        struct
        {
          UInt8 *first;
          UInt32 second;
        };

        MCPEntry(const UInt8* data, UInt32 size)
        {
          first = new UInt8[size];
          memcpy(first, data, size);
          second = size;
        }

        ~MCPEntry()
        {
          // It is the responsibility of whoever receives the data
          // to delete it (see recvFromMCPHelper...)
          //delete [] first;
        }
      };

      typedef struct MCPQueue
      {
         queue < MCPEntry*, deque<MCPEntry*> > q;
	 PIN_LOCK q_lock;
      } MCPQueue;

      static MCPQueue *mcp_queue;
      static Futex *mcp_futx;

      // mcp_idx is the mcp_queue entry used to send messages to the MCP
      static UInt32 mcp_idx;
      bool i_am_the_MCP;

      //***** Miscellaneous *****//
      static UINT32 s_pt_num_mod; // used for bounds checking only
	
   public:	
      int ptInit(int tid, int num_mod);

      // The MCP should use this initialization routine instead of ptInit
      void ptInitMCP() { i_am_the_MCP = true; }

      // This does nothing but is needed so the interface matches the
      //  other versions of the PT layer
      static void ptFinish() {}

      // CommID and ThreadID are the same in this version of the PT layer
      int ptCommID() { return pt_tid; }
      int ptSend(int receiver, char *buffer, int length);
      char* ptRecv();
      bool ptQuery();
      static void ptInitQueue(int num_mod);

      // These routines are used to communicate with the central server
      //  process (known as the "MCP").  There is exactly one server for
      //  the entire simulation but the user shouldn't have to know
      //  anything about where it is or how to get to it.
      void   ptSendToMCP(UInt8* buffer, UInt32 num_bytes);
      UInt8* ptRecvFromMCP(UInt32* num_bytes);
      // These two should only be called from the MCP
      void   ptMCPSend(UInt32 dest, UInt8* buffer, UInt32 num_bytes);
      UInt8* ptMCPRecv(UInt32* num_bytes);

   private:
      void ptMCPSendHelper(UInt32 dest, UInt8* buffer, UInt32 num_bytes);
      UInt8* ptMCPRecvHelper(UInt32 my_idx, UInt32* num_bytes);
};

#endif


