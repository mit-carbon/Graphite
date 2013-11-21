#ifndef __DSENTElectricalLink_H__
#define __DSENTElectricalLink_H__

#include "Type.h"
#include <vector>

namespace dsent_contrib
{
    class DSENTInterface;

    // This class is responsible for calling DSENT for a repeated electrical link
    class DSENTElectricalLink
    {
        public:
            // Create a DSENT run for a link with these parameters
            DSENTElectricalLink(double frequency_, double voltage_, double link_len_, unsigned int flit_width_,
                const DSENTInterface* dsent_interface_);
            ~DSENTElectricalLink();

        public:
            // Calculate dynamic energy consumed
            inline double calc_dynamic_energy(unsigned int num_flits_) const { return (num_flits_ * m_dynamic_energy_per_flit_); }
            // Get the static power consumption
            inline double get_static_power() const { return m_static_power_; }
            // Print information
            void print() const;

        private:
            // Helper function that actually calls DSENT
            void init(double frequency_, double voltage_, double link_len_, unsigned int flit_width_,
                const DSENTInterface* dsent_interface_);

        private:
            // Energy dissipated per flit
            double m_dynamic_energy_per_flit_;
            // Static power dissipation
            double m_static_power_;
    };
}

#endif

