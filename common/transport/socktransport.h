#ifndef SOCK_TRANSPORT_H
#define SOCK_TRANSPORT_H

#include "transport.h"

#include <list>

class SockTransport : public Transport
{
public:
   SockTransport();
   ~SockTransport();
   
   class SockNode : public Node
   {
   public:
      SockNode(core_id_t core_id, SockTransport *trans);
      ~SockNode();

      void globalSend(SInt32 dest_proc, const void *buffer, UInt32 length);
      void send(core_id_t dest_core, const void *buffer, UInt32 length);
      Byte* recv();
      bool query();

   private:
      void send(SInt32 dest_proc, UInt32 tag, const void *buffer, UInt32 length);

      SockTransport *m_transport;
   };

   Node *createNode(core_id_t core_id);

   void barrier();
   Node *getGlobalNode();

private:
   void getProcInfo();
   void initSockets();
   void initBufferLists();
   void updateBufferLists();

   struct Packet
   {
      UInt32 length;
      SInt32 tag;
      Byte data;
   };

   class Socket
   {
   public:
      Socket();
      ~Socket();

      // server api
      void listen(SInt32 port, SInt32 max_pending);
      Socket accept();

      // client api
      void connect(const char *addr, SInt32 port);

      void send(const void* buffer, UInt32 length);
      bool recv(void *buffer, UInt32 length, bool block);

      void close();

   private:
      Socket(SInt32);

      SInt32 m_socket;
   };

   Node *m_global_node;

   SInt32 m_num_procs;
   SInt32 m_proc_index;

   Socket m_server_socket;
   Lock *m_recv_locks;
   Socket *m_recv_sockets;
   Socket *m_send_sockets;

   static const int BASE_PORT = 2000;
   static const int BUFFER_SIZE = 0x10000;
   static const int GLOBAL_TAG = -1;

   typedef std::list<Byte*> buffer_list;
   SInt32 m_num_lists;
   buffer_list *m_buffer_lists;
   Lock *m_buffer_locks;
};

#endif // SOCK_TRANSPORT_H
