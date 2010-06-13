#pragma once

#include "SIM_parameter.h"
#include "SIM_router.h"

class OrionRouter
{
  public:
    OrionRouter(
      u_int num_input_port_,
      u_int num_output_port_,
      u_int num_vclass_,
      u_int num_vchannel_,
      u_int num_buf_,
      u_int flit_width_bit_
    );
    ~OrionRouter();

  public:
    void init(
      u_int num_input_port_,
      u_int num_output_port_,
      u_int num_vclass_,
      u_int num_vchannel_,
      u_int num_buf_,
      u_int flit_width_bit_
    );

    double calc_dynamic_energy_buf(bool is_read_);
    double calc_dynamic_energy_xbar(u_int num_bit_flips_);
    double calc_dynamic_energy_local_vc_arb(u_int num_req_);
    double calc_dynamic_energy_global_vc_arb(u_int num_req_);
    double calc_dynamic_energy_vc_select(bool is_read_);
    double calc_dynamic_energy_local_sw_arb(u_int num_req_);
    double calc_dynamic_energy_global_sw_arb(u_int num_req_);

    double get_static_power_buf() const { return m_static_power_buf; }
    double get_static_power_xbar() const { return m_static_power_xbar; }
    double get_static_power_sa() const { return m_static_power_sa; }
    double get_static_power_va() const { return m_static_power_va; }
    double get_static_power_clock() const { return m_static_power_clock; }

    double get_dynamic_energy_clock() const { return m_dynamic_energy_clock; }

  private:
    SIM_router_info_t m_router_info;
    SIM_router_power_t m_router_power;
    SIM_router_area_t m_router_area;

    double m_dynamic_energy_buf_read;
    double m_dynamic_energy_buf_write;
    double m_static_power_buf;

    double m_dynamic_energy_xbar;
    double m_static_power_xbar;

    double m_static_power_sa;

    double m_dynamic_energy_vc_select_read;
    double m_dynamic_energy_vc_select_write;
    double m_static_power_va;

    double m_static_power_clock;
    double m_dynamic_energy_clock;

    double m_dynamic_energy_link;
    double m_static_power_link;
};

class OrionLink
{
  public:
    OrionLink(
      double link_length_,
      u_int flit_width_bit_
    );
    ~OrionLink();

  public:
    void init(
      double link_length_,
      u_int flit_width_bit_
    );

    double calc_dynamic_energy(u_int num_bit_flip_);
    double get_static_power() const { return m_static_power; }

  private:
    double m_link_length;
    u_int m_flit_width_bit;

    double m_dynamic_energy_per_bit;
    double m_static_power;
};
