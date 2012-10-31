#include "DSENTInterface.h"
#include "dsent-core/libutil/LibUtil.h"
#include "dsent-core/DSENT.h"

#include <iostream>
#include <ostream>
#include <cassert>
#include <cstring>

using namespace std;
using namespace LibUtil;

namespace dsent_contrib
{
    DSENTInterface* DSENTInterface::m_singleton = NULL;

    const String DSENTInterface::el_link_cfg_file_name = "electrical-link.cfg";
    const String DSENTInterface::op_link_cfg_file_name = "optical-link.cfg";
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

        // Some hardcoded elements to this, depending on when graphite is able to parametrize the other parts
        if (tech_node_ == 45) m_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk45LVT.model";
        else if (tech_node_ == 32) m_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk32LVT.model";
        else if (tech_node_ == 22) m_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "Bulk22LVT.model";
        else if (tech_node_ == 11) m_tech_file_path_ = dsent_path_ + "/dsent-core/tech/tech_models/" + "TG11LVT.model";
        else assert(false);

    }

    DSENTInterface::~DSENTInterface()
    {}

    vector<String> DSENTInterface::run_dsent(const String& cfg_file_path_, const vector<String>& evals_, const vector<Overwrite>& overwrites_) const
    {
        // Create String stream to pass to DSENT
        ostringstream dsent_ost;
        // Create DSENT arguments vector
        vector<String>* dsent_args = new vector<String>();
        // Overwrites string
        String overrides_str = "";
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

        // Begin overrides
        dsent_args->push_back("-overwrite");

        // Add electrical technology model filename
        overrides_str = "ElectricalTechModelFilename=" + get_tech_file_path() + ";";

        // Add other overrides
        for (vector<Overwrite>::const_iterator it = overwrites_.begin(); it != overwrites_.end(); it++)
            overrides_str += " " + it->get_string() + ";" ;

        // Append full overrides string
        dsent_args->push_back(overrides_str);

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

