#ifndef LCP_H
#define LCP_H

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