#ifndef LCP_H
#define LCP_H

#include "thread.h"
#include "network.h"

class LCP : public Runnable
{
public:
   LCP();
   ~LCP();

   void run();

private:
   static void updateCommMap(void *vp, NetPacket pkt);
};

#endif // LCP_H
