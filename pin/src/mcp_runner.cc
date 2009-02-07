#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

#include "mcp_runner.h"
#include "core_manager.h"
#include "config.h"

#include "log.h"
#define LOG_DEFAULT_RANK g_config->getMCPCoreNum()
#define LOG_DEFAULT_MODULE MCP

MCPRunner::MCPRunner(MCP *mcp)
    : m_mcp(mcp)
{
}

void MCPRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
    //FIXME: this should probably be total cores, but that was returning
    //zero when I tried it. --cg3
    int tid =  syscall( __NR_gettid );
    LOG_PRINT("Initializing the MCP (%i) with id: %i", (int)tid, g_config->getTotalCores()-1);
    g_core_manager->initializeThread(g_config->getTotalCores()-1);

    while( !m_mcp->finished() )
    {
        m_mcp->run();
    }
}

