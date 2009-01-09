#ifndef MCP_API_H
#define MCP_API_H

// This is the dummy function that will get replaced
// by the simulator
void* mcp_thread_func(void *dummy);

// This is the routine that we instrument in order to
// send the final exit message to the MCP so that
// it will close properly.
void finishMCP();

void initMCP();
void quitMCP();



#endif
