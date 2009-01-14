#include "mcp_runner.h"
#include "chip.h"

MCPRunner::MCPRunner(MCP *mcp)
    : m_mcp(mcp)
{
}

void MCPRunner::RunThread(OS_SERVICES::ITHREAD *me)
{
    //FIXME: this should probably be total cores, but that was returning
    //zero when I tried it. --cg3

    cerr << "Initializing the MCP (" << (int)chipThreadId() << ") with id: " << g_knob_num_cores << endl;
    chipInit(g_knob_num_cores);

    while( !m_mcp->finished() )
    {
        m_mcp->run();
    }
}

