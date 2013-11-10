#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "fixed_types.h"

class Transport
{
public:
   virtual ~Transport() { };

   class Node
   {
   public:
      virtual ~Node() { }

      virtual void globalSend(SInt32 dest_proc, const void *buffer, UInt32 length) = 0;
      virtual void send(tile_id_t dest, const void *buffer, UInt32 length) = 0;
      virtual Byte* recv() = 0;
      virtual bool query() = 0;

   protected:
      tile_id_t getTileId();
      Node(tile_id_t tile_id);

   private:
      tile_id_t m_tile_id;
   };

   static Transport* create();
   static Transport* getSingleton();

   virtual Node* createNode(tile_id_t tile_id) = 0;

   virtual void barrier() = 0;
   virtual Node* getGlobalNode() = 0; // for communication not linked to a tile

protected:
   Transport();

private:
   static Transport *m_singleton;
};

#endif // TRANSPORT_H

