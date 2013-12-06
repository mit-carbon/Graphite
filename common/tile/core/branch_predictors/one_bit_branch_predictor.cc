#include "simulator.h"
#include "one_bit_branch_predictor.h"

OneBitBranchPredictor::OneBitBranchPredictor(UInt32 size)
   : m_bits(size)
{
}

OneBitBranchPredictor::~OneBitBranchPredictor()
{
}

bool OneBitBranchPredictor::predict(IntPtr ip, IntPtr target)
{
   UInt32 index = ip % m_bits.size();
   return m_bits[index];
}

void OneBitBranchPredictor::update(bool predicted, bool actual, IntPtr ip, IntPtr target)
{
   updateCounters(predicted, actual);
   UInt32 index = ip % m_bits.size();
   m_bits[index] = actual;
}

void OneBitBranchPredictor::outputSummary(std::ostream &os)
{
   BranchPredictor::outputSummary(os);
   os << "      Type: one-bit (" << m_bits.size() << ")" << endl;
}
