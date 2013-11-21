#ifndef __DSENTROUTER_H__
#define __DSENTROUTER_H__

#include "Type.h"

namespace dsent_contrib
{
    // This class is responsible for calling DSENT for a router
    class DSENTInterface;

    class DSENTRouter
    {
        public:
            // Create a DSENT run for a router with these parameters
            DSENTRouter(
                double frequency_,
                double voltage_,
                unsigned int num_in_port_,
                unsigned int num_out_port_,
                unsigned int num_vclass_,
                unsigned int num_vchannel_,
                unsigned int num_buffers_,
                unsigned int flit_width_,
                const DSENTInterface* dsent_interface_
            );
        ~DSENTRouter();

        public:
            // Get dynamic energy components
            inline double calc_dynamic_energy_buf_write(double num_req_) const { return (num_req_ * m_dynamic_energy_buf_write_); }
            inline double calc_dynamic_energy_buf_read(double num_req_) const { return (num_req_ * m_dynamic_energy_buf_read_); }
            inline double calc_dynamic_energy_sa(double num_req_) const { return (num_req_ * m_dynamic_energy_sa_); }
            inline double calc_dynamic_energy_xbar(double num_req_, unsigned int multicast_idx) const { return (num_req_ * m_dynamic_energy_xbar_->at(multicast_idx)); }
            inline double calc_dynamic_energy_clock(double num_events_) const { return (num_events_ * m_dynamic_energy_clock_); }

            // Get static energy components
            inline double get_static_power_buf() const { return m_static_power_buf_; }
            inline double get_static_power_sa() const { return m_static_power_sa_; }
            inline double get_static_power_xbar() const { return m_static_power_xbar_; }
            inline double get_static_power_clock() const { return m_static_power_clock_; }

            void print() const;

        private:
            void init(
                double frequency_,
                double voltage_,
                unsigned int num_in_port_,
                unsigned int num_out_port_,
                unsigned int num_vclass_,
                unsigned int num_vchannel_,
                unsigned int num_buffers_,
                unsigned int flit_width_,
                const DSENTInterface* dsent_interface_
            );
            
        private:
            // Dynamic energy components
            double m_dynamic_energy_buf_write_;
            double m_dynamic_energy_buf_read_;
            double m_dynamic_energy_sa_;
            double m_dynamic_energy_clock_;
            vector<double>* m_dynamic_energy_xbar_;
        
            // Static power components
            double m_static_power_buf_;
            double m_static_power_xbar_;
            double m_static_power_sa_;
            double m_static_power_clock_;

    };
}

#endif

