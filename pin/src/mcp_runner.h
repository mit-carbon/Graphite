
#include "os-services.hpp"
#include "mcp.h"

class MCPRunner : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
       MCPRunner(MCP *mcp);
       virtual void RunThread(OS_SERVICES::ITHREAD *me);
   private:
       MCP *m_mcp;
};
