#include "classifier.h"
#include "classifiers/private_fixed_classifier.h"
#include "classifiers/remote_fixed_classifier.h"
#include "classifiers/random_classifier.h"
#include "classifiers/shared_remote_classifier.h"
#include "classifiers/shared_read_write_remote_classifier.h"
#include "classifiers/locality_based_classifier.h"
#include "classifiers/predictive_locality_based_classifier.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

Classifier::Type Classifier::_type;

Classifier::Classifier()
{}

Classifier::~Classifier()
{}

Classifier::Type
Classifier::parse(string type)
{
   if (type == "private_fixed")
      return PRIVATE_FIXED;
   else if (type == "remote_fixed")
      return REMOTE_FIXED;
   else if (type == "random_line_based")
      return RANDOM_LINE_BASED;
   else if (type == "random_sharer_based")
      return RANDOM_SHARER_BASED;
   else if (type == "shared_remote")
      return SHARED_REMOTE;
   else if (type == "shared_read_write_remote")
      return SHARED_READ_WRITE_REMOTE;
   else if (type == "locality_based")
      return LOCALITY_BASED;
   else if (type == "predictive_locality_based")
      return PREDICTIVE_LOCALITY_BASED;
   else
      LOG_PRINT_ERROR("Unrecognized classifier type (%s)", type.c_str());
   return NUM_TYPES;
}

Classifier*
Classifier::create(SInt32 max_hw_sharers)
{
   switch (_type)
   {
   case PRIVATE_FIXED:
      return new PrivateFixedClassifier();
   case REMOTE_FIXED:
      return new RemoteFixedClassifier();
   case RANDOM_LINE_BASED:
      return new RandomClassifier(LINE_BASED);
   case RANDOM_SHARER_BASED:
      return new RandomClassifier(SHARER_BASED);
   case SHARED_REMOTE:
      return new SharedRemoteClassifier();
   case SHARED_READ_WRITE_REMOTE:
      return new SharedReadWriteRemoteClassifier();
   case LOCALITY_BASED:
      return new LocalityBasedClassifier(max_hw_sharers);
   case PREDICTIVE_LOCALITY_BASED:
      return new PredictiveLocalityBasedClassifier();
   default:
      LOG_PRINT_ERROR("Unrecognized classifier type(%u)", _type);
      return (Classifier*) NULL;
   }
}

}
