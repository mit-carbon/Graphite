#ifndef __DSENT_DSENT_H__
#define __DSENT_DSENT_H__

// For DSENT operations
#include <iostream>

#include "libutil/OptionParser.h"
#include "libutil/Calculator.h"
#include "libutil/String.h"

namespace DSENT
{
    using std::ostream;

    using LibUtil::OptionParser;
    using LibUtil::Calculator;
    using LibUtil::String;
    
    class Model;
    
    class DSENT
    {
        protected:
            class DSENTCalculator : public Calculator
            {
                public:
                    DSENTCalculator();
                    virtual ~DSENTCalculator();
        
                protected:
                    virtual double getEnvVar(const String& var_name_) const;
            }; // class DSENTCalculator

        public:
            // A full execution of DSENT, with output directed to the specified
            // output stream
            static void run(int argc_, char** argv_, ostream& ost_);

        protected:
            static void setRuntimeOptions(OptionParser* option_parser_);
            static void initialize(int argc_, char** argv_, ostream& ost_);
            static void buildModel(ostream& ost_);
            static void processQuery(ostream& ost_);
            static const void* processQuery(const String& query_str_, bool is_print_, ostream& ost_);
            static void finalize();

            static void performTimingOpt();
            static void reportTiming(ostream& ost_);

            static void processEvaluate(ostream& ost_);

        protected:
            static Model* ms_model_;

            static bool ms_is_verbose_;

    }; // class DSENT

} // namespace DSENT

#endif // __DSENT_DSENT_H__

