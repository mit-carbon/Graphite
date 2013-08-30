#include "DSENTInterface.h"
#include "dsent-core/libutil/LibUtil.h"
#include "dsent-core/DSENT.h"

#include <iostream>
#include <ostream>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;
using namespace LibUtil;

namespace dsent_contrib
{
   DSENTInterface* DSENTInterface::m_singleton = NULL;

   const String DSENTInterface::el_link_cfg_file_name = "electrical-link.cfg";
   const String DSENTInterface::op_link_cfg_file_name = "photonic-link.cfg";
   const String DSENTInterface::router_cfg_file_name = "router.cfg";

   DSENTInterface::Overwrite::Overwrite(const String& var_, const String& val_) :
      m_var_(var_), m_val_(val_)
   {}

   DSENTInterface::Overwrite::~Overwrite()
   {}

   void DSENTInterface::allocate(const string& dsent_path_, unsigned int tech_node_)
   {
      assert(!m_singleton);
      m_singleton = new DSENTInterface(String(dsent_path_), tech_node_);
   }

   void DSENTInterface::release()
   {
      assert(m_singleton);
      delete m_singleton;
   }

   DSENTInterface* DSENTInterface::getSingleton()
   {
      assert(m_singleton);
      return m_singleton;
   }

   DSENTInterface::DSENTInterface(const String& dsent_path_, unsigned int tech_node_)
   {
      m_el_link_cfg_file_path_ = dsent_path_ + "/dsent-core/configs/" + el_link_cfg_file_name;
      m_op_link_cfg_file_path_ = dsent_path_ + "/dsent-core/configs/" + op_link_cfg_file_name;
      m_router_cfg_file_path_ = dsent_path_ + "/dsent-core/configs/" + router_cfg_file_name;

      // Photonics techfile
      m_phot_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Photonics.model";
        
      // Some hardcoded elements to this, depending on when graphite is able to parametrize the other parts
      if (tech_node_ == 45) m_elec_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk45LVT.model";
      else if (tech_node_ == 32) m_elec_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk32LVT.model";
      else if (tech_node_ == 22) m_elec_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk22LVT.model";
      else if (tech_node_ == 11) m_elec_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "TG11LVT.model";
      else assert(false);

      // Create global tech overwrites
      m_overwrites_tech_ = new vector<Overwrite>();

      // Get the database filename & library name
      string dsent_libname = dsent_path_ + "/libdsent_contrib.a";
      DBUtils::initialize(m_database, "dsent", dsent_libname);
   }

   DSENTInterface::~DSENTInterface()
   {
      delete m_overwrites_tech_;
   }

   void DSENTInterface::add_global_tech_overwrite(const String& var_, const String& val_)
   {
      m_overwrites_tech_->push_back(Overwrite(var_, val_)); 
   }

   vector<String> DSENTInterface::run_dsent(const String& cfg_file_path_, const vector<String>& evals_,
       const vector<Overwrite>& overwrites_, const vector<Overwrite>& overwrites_tech_) const
   {
      // Create String stream to pass to DSENT
      ostringstream dsent_ost;
      // Create DSENT arguments vector
      vector<String>* dsent_args = new vector<String>();
      // Overwrites string
      String overwrites_str = "";
      // Tech overwrites string
      String overwrites_tech_str = "";
      // DSENT output string
      String dsent_out_string;

      // Specify the correct config file path
      dsent_args->push_back("-cfg");
      dsent_args->push_back(cfg_file_path_);

      // Clear any queries
      dsent_args->push_back("-query");
      dsent_args->push_back(" ");

      // Create evaluate String
      if (!evals_.empty())
      {
         String eval_str = "";
         for (vector<String>::const_iterator it = evals_.begin(); it != evals_.end(); it++)
            eval_str += "print " + *it + ";";
         dsent_args->push_back("-eval");
         dsent_args->push_back(eval_str);
      }

      // Begin overwrites
      dsent_args->push_back("-overwrite");

      // Add electrical technology model filename
      overwrites_str = "ElectricalTechModelFilename=" + get_elec_tech_file_path() + ";";
      overwrites_str += " PhotonicTechModelFilename=" + get_phot_tech_file_path() + ";";
      // Add other overwrites
      for (vector<Overwrite>::const_iterator it = overwrites_.begin(); it != overwrites_.end(); it++)
         overwrites_str += " " + it->get_string() + ";" ;

      // Append full overwrites string
      dsent_args->push_back(overwrites_str);

      // Begin tech overwrites
      dsent_args->push_back("-overwrite_tech");

      // Add global tech overwrites
      for (vector<Overwrite>::const_iterator it = m_overwrites_tech_->begin(); it != m_overwrites_tech_->end(); it++)
         overwrites_tech_str += " " + it->get_string() + ";" ;
      // Add local tech overwrites
      for (vector<Overwrite>::const_iterator it = overwrites_tech_.begin(); it != overwrites_tech_.end(); it++)
         overwrites_tech_str += " " + it->get_string() + ";" ;
         
      // Append full tech overwrites string
      dsent_args->push_back(overwrites_tech_str);

      // Convert arguments vector to char**
      char** dsent_args_raw = new char*[dsent_args->size()];    
      for (unsigned int i = 0; i < dsent_args->size(); i++)
      {
         // Probably should check this is doing the right things
         dsent_args_raw[i] = new char[dsent_args->at(i).size()+1];
         strcpy(dsent_args_raw[i], dsent_args->at(i).c_str());
      }

      // Run DSENT
      DSENT::DSENT::run(int(dsent_args->size()), dsent_args_raw, dsent_ost);

      // Get DSENT output into a String
      dsent_out_string = dsent_ost.str();

      // Delete stuff
      for (unsigned int i = 0; i < dsent_args->size(); i++)
         delete dsent_args_raw[i];
      delete dsent_args_raw;
      delete dsent_args;

      // Return split line-by-line outputs
      return dsent_out_string.split("\n");
   }
}
