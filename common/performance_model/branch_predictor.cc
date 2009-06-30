#include "simulator.h"
#include "branch_predictor.h"
#include "one_bit_branch_predictor.h"

BranchPredictor::BranchPredictor()
   : m_correct_predictions(0)
   , m_incorrect_predictions(0)
{ }

BranchPredictor::~BranchPredictor()
{ }

UInt64 BranchPredictor::m_mispredict_penalty;

BranchPredictor* BranchPredictor::create()
{
   try
   {
      config::Config *cfg = Sim()->getCfg();
      assert(cfg);

      m_mispredict_penalty = cfg->getInt("perf_model/branch_predictor/mispredict_penalty",0);

      string type = cfg->getString("perf_model/branch_predictor/type","none");
      if (type == "none")
      {
         return 0;
      }
      else if (type == "one_bit")
      {
         UInt32 size = cfg->getInt("perf_model/branch_predictor/size");
         return new OneBitBranchPredictor(size);
      }
      else
      {
         LOG_PRINT_ERROR("Invalid branch predictor type.");
         return 0;
      }
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Config info not available while constructing branch predictor.");
      return 0;
   }
}

UInt64 BranchPredictor::getMispredictPenalty()
{
   return m_mispredict_penalty;
}

void BranchPredictor::updateCounters(bool predicted, bool actual)
{
   if (predicted == actual)
      ++m_correct_predictions;
   else
      ++m_incorrect_predictions;
}

void BranchPredictor::outputSummary(std::ostream &os)
{
   os << "  Branch predictor stats:" << endl
      << "    num correct:   " << m_correct_predictions << endl
      << "    num incorrect: " << m_incorrect_predictions << endl;
}
