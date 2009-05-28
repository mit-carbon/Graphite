#ifndef __MESSAGE_TYPES_H__
#define __MESSAGE_TYPES_H__

// Different types of messages that get passed to the MCP
typedef enum
{
   MCP_MESSAGE_QUIT,
   MCP_MESSAGE_SYS_CALL,
   MCP_MESSAGE_MUTEX_INIT,
   MCP_MESSAGE_MUTEX_LOCK,
   MCP_MESSAGE_MUTEX_UNLOCK,
   MCP_MESSAGE_COND_INIT,
   MCP_MESSAGE_COND_WAIT,
   MCP_MESSAGE_COND_SIGNAL,
   MCP_MESSAGE_COND_BROADCAST,
   MCP_MESSAGE_BARRIER_INIT,
   MCP_MESSAGE_BARRIER_WAIT,
   MCP_MESSAGE_UTILIZATION_UPDATE,
   MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER,
   MCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE,
   MCP_MESSAGE_THREAD_EXIT,
   MCP_MESSAGE_THREAD_JOIN_REQUEST,
   MCP_MESSAGE_QUIT_THREAD_SPAWNER,
   MCP_MESSAGE_QUIT_THREAD_SPAWNER_ACK,
} MCPMessageTypes;

typedef enum
{
   LCP_MESSAGE_QUIT,
   LCP_MESSAGE_COMMID_UPDATE,
   LCP_MESSAGE_QUIT_THREAD_SPAWNER,
   LCP_MESSAGE_QUIT_THREAD_SPAWNER_ACK,
   LCP_MESSAGE_SIMULATOR_FINISHED,
   LCP_MESSAGE_SIMULATOR_FINISHED_ACK,
   LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_MASTER,
} LCPMessageTypes;

#endif

