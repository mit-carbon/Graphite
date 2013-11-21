#ifndef __DSENTOPTICALLINK_H__
#define __DSENTOPTICALLINK_H__

#include "Type.h"

namespace dsent_contrib
{
    // This class is responsible for calling DSENT for a router
    class DSENTInterface;

    class DSENTOpticalLink
    {

        public:
            // Create a DSENT run for a router with these parameters
            DSENTOpticalLink(
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
            );
        ~DSENTOpticalLink();

        public:
            // Get dynamic energy components
            inline double calc_dynamic_energy(double num_flits_, unsigned int num_readers_) const { return (num_flits_ * m_dynamic_energy_->at(num_readers_)); }

            // Get static energy components
            inline double get_static_power_leakage() const { return m_static_power_leakage_; }
            inline double get_static_power_laser() const { return m_static_power_laser_; }
            inline double get_static_power_heating() const { return m_static_power_heating_; }

            void print() const;

        private:
            void init(
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
            );
            
        private:
            // Dynamic energy components
            vector<double>* m_dynamic_energy_;
            
            // Static power components
            double m_static_power_leakage_;
            double m_static_power_laser_;
            double m_static_power_heating_;
    };
}

#endif

