#ifndef MCP_API_H
#define MCP_API_H

// This is the dummy function that will get replaced
// by the simulator
extern "C" {

   // This is the routine that we instrument that 
   // handles the MCP code but the actual function
   // call comes from the userland thread context.
   void runMCP();

   // This is the routine that we instrument in order to
   // send the final exit message to the MCP so that
   // it will close properly.
   void finishMCP();
}

void initMCP();
void quitMCP();


void* mcp_thread_func(void *dummy);

#endif
