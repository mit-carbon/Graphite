#ifndef MCP_API_H
#define MCP_API_H

// This is the dummy function that will get replaced
// by the simulator
extern "C" {
   void runMCP();
}

void initMCP();
void quitMCP();

void* mcp_thread_func(void *dummy);

#endif
