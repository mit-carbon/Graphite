#ifndef MCP_RUNNER_H
#define MCP_RUNNER_H

#include "os-services.hpp"

class MCP;

class MCPRunner : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
      MCPRunner(MCP *mcp);
      virtual void RunThread(OS_SERVICES::ITHREAD *me);
   private:
      MCP *m_mcp;
};

MCPRunner * StartMCPThread();

#endif // MCP_RUNNER_H
