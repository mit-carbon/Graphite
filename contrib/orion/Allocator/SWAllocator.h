#ifndef __SWALLOCATOR_H__
#define __SWALLOCATOR_H__

#include "Type.h"

class TechParameter;
class OrionConfig;
class Arbiter;
class Crossbar;

class SWAllocator
{
  protected:
    SWAllocator(
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      double len_in_wire_,
      const string& local_arb_model_str_,
      const string& local_arb_ff_model_str_,
      const string& global_arb_model_str_,
      const string& global_arb_ff_model_str_,
      const TechParameter* tech_param_ptr_
    );

  public:
    ~SWAllocator();

  public:
    double get_dynamic_energy_local_sw_arb(double num_req_, bool is_max_) const;
    double get_dynamic_energy_global_sw_arb(double num_req_, bool is_max_) const;
    double get_static_power() const;

    void print_all() const;

  protected:
    void init();

  protected:
    uint32_t m_num_in_port;
    uint32_t m_num_out_port;
    uint32_t m_num_vclass;
    uint32_t m_num_vchannel;

    Arbiter* m_local_arb_ptr;
    Arbiter* m_global_arb_ptr;

  public:
    static SWAllocator* create_swallocator(
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      const Crossbar* xbar_ptr_,
      const OrionConfig* orion_cfg_ptr_
    );
};

#endif
