#ifndef BRANCH_PREDICTOR_H
#define BRANCH_PREDICTOR_H

#include <iostream>

#include "fixed_types.h"

class BranchPredictor
{
public:
   BranchPredictor();
   virtual ~BranchPredictor();

   virtual bool predict(IntPtr ip, IntPtr target) = 0;
   virtual void update(bool predicted, bool actual, IntPtr ip, IntPtr target) = 0;

   UInt64 getMispredictPenalty();
   static BranchPredictor* create();

   virtual void outputSummary(std::ostream &os);
   UInt64 getNumCorrectPredictions() { return m_correct_predictions; }
   UInt64 getNumIncorrectPredictions() { return m_incorrect_predictions; }

protected:
   void updateCounters(bool predicted, bool actual);

private:
   UInt64 m_correct_predictions;
   UInt64 m_incorrect_predictions;

   static UInt64 m_mispredict_penalty;

   void initializeCounters();
};

#endif
