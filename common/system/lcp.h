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
   void finish();

private:
   void processPacket();

   void updateCommId(void *vp);

   SInt32 m_proc_num;
   Transport::Node *m_transport;
   bool m_finished; // FIXME: this should really be part of the thread class somehow
};

#endif // LCP_H
