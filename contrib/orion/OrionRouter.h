#ifndef __ORIONROUTER_H__
#define __ORIONROUTER_H__

#include "Type.h"

class OrionConfig;
class Buffer;
class Crossbar;
class VCAllocator;
class SWAllocator;
class Clock;

class OrionRouter
{
  public:
    OrionRouter(
      const OrionConfig* orion_cfg_ptr_
    );
    //values in cfg file will be modified
    OrionRouter(
      float frequency_,
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      uint32_t in_buf_num_set_,
      uint32_t flit_width_,
      OrionConfig* orion_cfg_ptr_
    );
    ~OrionRouter();

  public:
    //double calc_dynamic_energy(double e_fin_, bool is_max_ = false) const;
    //double calc_dynamic_energy_in_buf(bool is_read_, bool is_max_ = false) const;
    //double calc_dynamic_energy_out_buf(bool is_read_, bool is_max_ = false) const;
    double calc_dynamic_energy_buf(bool is_read_, bool is_max_ = false) const;
    double calc_dynamic_energy_xbar(bool is_max_ = false) const;
    double calc_dynamic_energy_local_vc_arb(double num_req_, bool is_max_ = false) const;
    double calc_dynamic_energy_global_vc_arb(double num_req_, bool is_max_ = false) const;
    double calc_dynamic_energy_vc_select(bool is_read_, bool is_max_ = false) const;
    double calc_dynamic_energy_local_sw_arb(double num_req_, bool is_max_ = false) const;
    double calc_dynamic_energy_global_sw_arb(double num_req_, bool is_max_ = false) const;
    double calc_dynamic_energy_clock() const;

    double get_static_power_buf() const;
    double get_static_power_xbar() const;
    double get_static_power_va() const;
    double get_static_power_sa() const;
    double get_static_power_clock() const;

    void print() const;

  private:
    void init();

  private:
    const OrionConfig* m_orion_cfg_ptr;

    uint32_t m_num_in_port;
    uint32_t m_num_out_port;
    uint32_t m_flit_width;
    uint32_t m_num_vclass;
    uint32_t m_num_vchannel;
    bool m_is_in_shared_buf;
    bool m_is_out_shared_buf;
    bool m_is_in_shared_switch;
    bool m_is_out_shared_switch;

    Buffer* m_in_buf_ptr;
    Crossbar* m_xbar_ptr;
    VCAllocator* m_va_ptr;
    SWAllocator* m_sa_ptr;
    Clock* m_clk_ptr;
};

#endif

