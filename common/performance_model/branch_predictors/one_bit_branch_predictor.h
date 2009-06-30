#ifndef ONE_BIT_BRANCH_PREDICTOR_H
#define ONE_BIT_BRANCH_PREDICTOR_H

#include "branch_predictor.h"

#include <vector>

class OneBitBranchPredictor : public BranchPredictor
{
public:
   OneBitBranchPredictor(UInt32 size);
   ~OneBitBranchPredictor();

   bool predict(IntPtr ip, IntPtr target);
   void update(bool predicted, bool actual, IntPtr ip, IntPtr target);

   void outputSummary(std::ostream &os);

private:
   std::vector<bool> m_bits;
};

#endif
