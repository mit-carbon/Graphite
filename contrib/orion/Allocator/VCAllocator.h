#ifndef __VCALLOCATOR_H__
#define __VCALLOCATOR_H__

#include "Type.h"

class TechParameter;
class OrionConfig;
class Arbiter;
class Buffer;

class VCAllocator
{
  public:
    enum VAModel
    {
      NO_MODEL = 0,
      ONE_STAGE_ARB,
      TWO_STAGE_ARB,
      VC_SELECT
    };

  public:
    ~VCAllocator();

  protected:
    // for ONE_STAGE_ARB
    VCAllocator(
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      const string& arb_model_str_,
      const string& arb_ff_model_str_,
      const TechParameter* tech_param_ptr_
    );
    // for TWO_STAGE_ARB
    VCAllocator(
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      const string& local_arb_model_str_,
      const string& local_arb_ff_model_str_,
      const string& global_arb_model_str_,
      const string& global_arb_ff_model_str_,
      const TechParameter* tech_param_ptr_
    );
    // for VC_SELECT
    VCAllocator(
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      const string& vc_select_buf_model_str_,
      const OrionConfig* orion_cfg_ptr_  
    );

  public:
    double get_dynamic_energy_local_vc_arb(double num_req_, bool is_max_) const;
    double get_dynamic_energy_global_vc_arb(double num_req_, bool is_max_) const;
    double get_dynamic_energy_vc_select(bool is_read_, bool is_max_) const;
    double get_static_power() const;

  protected:
    VAModel m_va_model;
    uint32_t m_num_in_port;
    uint32_t m_num_out_port;
    uint32_t m_num_vclass;
    uint32_t m_num_vchannel;

    Arbiter* m_local_arb_ptr;
    Arbiter* m_global_arb_ptr;
    Buffer* m_vc_select_ptr;

  public:
    static VCAllocator* create_vcallocator(
      const string& vcalloc_model_str_,
      uint32_t num_in_port_,
      uint32_t num_out_port_,
      uint32_t num_vclass_,
      uint32_t num_vchannel_,
      const OrionConfig* orion_cfg_ptr_
    );
};

#endif
