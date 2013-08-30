#include "DSENTOpticalLink.h"
#include "DSENTInterface.h"
#include "dsent-core/libutil/String.h"
#include "../../common/misc/packetize.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

using namespace std;
using namespace LibUtil;

namespace dsent_contrib
{
    // Future TODO: There is currently a limitation in this interface that is preventing
    // us from getting the breakdowns in the dynamic energy from DSENT

    DSENTOpticalLink::DSENTOpticalLink(
            double core_data_rate_,
            double link_data_rate_,
            double voltage_,
            double length_,
            unsigned int num_readers_,
            unsigned int max_readers_,
            unsigned int core_flit_width_,
            const string& tuning_strategy_,
            const string& laser_type_,
            const DSENTInterface* dsent_interface_
        )
    {
        assert(core_data_rate_ > 0);
        assert(link_data_rate_ > 0);
        assert(voltage_ >= 0);
        assert(length_ >= 0);
        assert(core_flit_width_ > 0);
       
        // Get database
        DB* database = dsent_interface_->getDatabase();

        // Zero-out key, data
        DBT key, data;
        memset(&key, 0, sizeof(DBT));
        memset(&data, 0, sizeof(DBT));

        // Create key
        UnstructuredBuffer input;
        input << core_data_rate_ << link_data_rate_ << voltage_ << num_readers_ << max_readers_ << core_flit_width_
              << tuning_strategy_ << laser_type_;
        key.data = (char*) input.getBuffer();
        key.size = input.size();

        // Get dynamic_energy / static_power
        UnstructuredBuffer output;
        if (DBUtils::getRecord(database, key, data) == DB_NOTFOUND)
        {
            // Power of 2 serdes ratio check
            unsigned int serdes_ratio = (int) (link_data_rate_ / core_data_rate_);
            assert(((serdes_ratio - 1) & serdes_ratio) == 0);
           
            // Create vectors
            m_dynamic_energy_ = new vector<double>();

            // Initialize everything else
            init(core_data_rate_, link_data_rate_, voltage_, length_, num_readers_, max_readers_, core_flit_width_, 
                tuning_strategy_, laser_type_, dsent_interface_);

            // Create output
            for (unsigned int multicast = 0; multicast <= max_readers_; ++multicast)
                output << m_dynamic_energy_->at(multicast);
            output << m_static_power_leakage_ << m_static_power_laser_ << m_static_power_heating_;
            data.data = (char*) output.getBuffer();
            data.size = output.size();

            // Write in database
            DBUtils::putRecord(database, key, data);
        }
        else // (DBUtils::getRecord(database, key, data) == 0)
        {
            // Read from database
            output << make_pair(data.data, data.size);

            // Populate object fields
            m_dynamic_energy_ = new vector<double>(max_readers_+1);
            for (unsigned int multicast = 0; multicast <= max_readers_; ++multicast)
                output >> m_dynamic_energy_->at(multicast);
            output >> m_static_power_leakage_ >> m_static_power_laser_ >> m_static_power_heating_;            
        }
    }

    DSENTOpticalLink::~DSENTOpticalLink()
    {
        delete m_dynamic_energy_;        
        m_dynamic_energy_ = NULL;
    }

    void DSENTOpticalLink::init(
            double core_data_rate_,
            double link_data_rate_,
            double voltage_,
            double length_,
            unsigned int num_readers_,
            unsigned int max_readers_,
            unsigned int core_flit_width_,
            const string& tuning_strategy_,
            const string& laser_type_,
            const DSENTInterface* dsent_interface_
        )
    {
        // Create DSENT evaluate
        vector<String> eval = vector<String>();
        // Create DSENT overwrites vector
        vector<DSENTInterface::Overwrite> overwrites = vector<DSENTInterface::Overwrite>();
        // Create DSENT tech overwrites vector
        vector<DSENTInterface::Overwrite> overwrites_tech = vector<DSENTInterface::Overwrite>();
        // DSENT outputs
        vector<String> outputs;

        // Create evaluate String    
        // Dynamic power component
        for (unsigned int multicast = 1; multicast <= max_readers_; ++multicast)
            eval.push_back( "$(Energy>>SWMRLink:MulticastFlit" + (String) multicast + ")");

        // Static power components
        eval.push_back( "$(NddPower>>SWMRLink:Leakage)");
        eval.push_back( "$(NddPower>>SWMRLink:Laser)");
        eval.push_back( "$(NddPower>>SWMRLink:RingTuning)");

        // Create overwrites
        overwrites.push_back(DSENTInterface::Overwrite("CoreDataRate", String(core_data_rate_)));
        overwrites.push_back(DSENTInterface::Overwrite("LinkDataRate", String(link_data_rate_)));
        overwrites.push_back(DSENTInterface::Overwrite("Length", String(length_)));
        overwrites.push_back(DSENTInterface::Overwrite("NumberReaders", String(num_readers_)));
        overwrites.push_back(DSENTInterface::Overwrite("MaxReaders", String(max_readers_))); 
        overwrites.push_back(DSENTInterface::Overwrite("NumberBits", String(core_flit_width_))); 
        overwrites.push_back(DSENTInterface::Overwrite("RingTuningMethod", String(tuning_strategy_))); 
        overwrites.push_back(DSENTInterface::Overwrite("LaserType", String(laser_type_))); 

        // Create tech overwrites
        overwrites_tech.push_back(DSENTInterface::Overwrite("Vdd", String(voltage_)));

        // Run DSENT
        outputs = dsent_interface_->run_dsent(dsent_interface_->get_op_link_cfg_file_path(),
            eval, overwrites, overwrites_tech);

        // Check to make sure we get the expected number of outputs
        assert(outputs.size() == 3 + max_readers_);

        // Store outputs
        // Dynamic energy components
        // Multicast energies (add a 0 for a multicast to 0 readers)
        m_dynamic_energy_->push_back(0.0);
        for (unsigned int multicast = 1; multicast <= max_readers_; ++multicast)
            m_dynamic_energy_->push_back(outputs[multicast - 1]);

        // Static power components
        m_static_power_leakage_ = outputs[0 + max_readers_];
        m_static_power_laser_ = outputs[1 + max_readers_];
        m_static_power_heating_ = outputs[2 + max_readers_];

        return;
    }

    void DSENTOpticalLink::print() const
    {
        cout << "Optical Link - Dynamic Energy" << endl;
        for (unsigned int multicast = 0; multicast < m_dynamic_energy_->size(); ++multicast)
            cout << "\t" << "Multicast " << multicast << " = " << calc_dynamic_energy(1, multicast) << endl;
        cout << endl;
        cout << "Optical Link - Static Power" << endl;
        cout << "\t" << "Leakage = " << get_static_power_leakage() << endl;
        cout << "\t" << "Laser = " << get_static_power_laser() << endl;
        cout << "\t" << "Heating = " << get_static_power_heating() << endl;
        cout << endl;
        return;
    }
}

