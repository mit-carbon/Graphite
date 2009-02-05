#include "mcp_runner.h"
#include "chip.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK g_config->MCPCoreNum()
#define LOG_DEFAULT_MODULE MCP

MCPRunner::MCPRunner(MCP *mcp)
    : m_mcp(mcp)
{
}

void MCPRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
    //FIXME: this should probably be total cores, but that was returning
    //zero when I tried it. --cg3

    LOG_PRINT("Initializing the MCP (%i) with id: %i", (int)chipThreadId(), g_config->totalCores()-1);
    chipInit(g_config->totalCores()-1);

    while( !m_mcp->finished() )
    {
        m_mcp->run();
    }
}

