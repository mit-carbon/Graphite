#include "directory_type.h"
#include "classifier.h"
#include "fixed_types.h"

namespace HybridProtocol_PPMOSI_SS
{

DirectoryEntry* createAdaptiveDirectoryEntry(DirectoryType directory_type, SInt32 max_hw_sharers, SInt32 max_num_sharers);

class AdaptiveDirectoryEntry
{
public:
   AdaptiveDirectoryEntry(SInt32 max_hw_sharers);
   virtual ~AdaptiveDirectoryEntry();

   Classifier* getClassifier() { return _classifier; }

private:
   Classifier* _classifier;
};

}
