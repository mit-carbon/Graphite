#include <stdlib.h>
#include <iostream>

#include "VCAllocator.h"
#include "Util.h"
#include "OrionConfig.h"
#include "Arbiter.h"
#include "Buffer.h"

using namespace std;

VCAllocator::VCAllocator(
  uint32_t num_in_port_,
  uint32_t num_out_port_,
  uint32_t num_vclass_,
  uint32_t num_vchannel_,
  const string& arb_model_str_,
  const string& arb_ff_model_str_,
  const TechParameter* tech_param_ptr_
)
{
  m_va_model = ONE_STAGE_ARB;
  m_num_in_port = num_in_port_;
  m_num_out_port = num_out_port_;
  m_num_vclass = num_vclass_;
  m_num_vchannel = num_vchannel_;

  m_local_arb_ptr = NULL;

  m_global_arb_ptr = Arbiter::create_arbiter(
    arb_model_str_, arb_ff_model_str_,
    (m_num_in_port-1)*m_num_vchannel, 0, tech_param_ptr_);

  m_vc_select_ptr = NULL;
}

VCAllocator::VCAllocator(
  uint32_t num_in_port_,
  uint32_t num_out_port_,
  uint32_t num_vclass_,
  uint32_t num_vchannel_,
  const string& local_arb_model_str_,
  const string& local_arb_ff_model_str_,
  const string& global_arb_model_str_,
  const string& global_arb_ff_model_str_,
  const TechParameter* tech_param_ptr_
)
{
  m_va_model = TWO_STAGE_ARB;
  m_num_in_port = num_in_port_;
  m_num_out_port = num_out_port_;
  m_num_vclass = num_vclass_;
  m_num_vchannel = num_vchannel_;

  // first stage
  m_local_arb_ptr = Arbiter::create_arbiter(
    local_arb_model_str_, local_arb_ff_model_str_,
    m_num_vchannel, 0, tech_param_ptr_);

  // second stage
  m_global_arb_ptr = Arbiter::create_arbiter(
    global_arb_model_str_, global_arb_ff_model_str_,
    (m_num_in_port-1)*m_num_vchannel, 0, tech_param_ptr_);

  m_vc_select_ptr = NULL;
}

VCAllocator::VCAllocator(
  uint32_t num_in_port_,
  uint32_t num_out_port_,
  uint32_t num_vclass_,
  uint32_t num_vchannel_,
  const string& vc_select_buf_model_str_,
  const OrionConfig* orion_cfg_ptr_  
)
{
  m_va_model = VC_SELECT;
  m_num_in_port = num_in_port_;
  m_num_out_port = num_out_port_;
  m_num_vclass = num_vclass_;
  m_num_vchannel = num_vchannel_;

  m_local_arb_ptr = NULL;
  m_global_arb_ptr = NULL;

  uint32_t vc_select_buf_num_set = m_num_vchannel;
  uint32_t vc_select_buf_line_width = (uint32_t)ceil(log2(m_num_vchannel));
  m_vc_select_ptr = new Buffer(vc_select_buf_model_str_, true, false, 
    vc_select_buf_num_set, vc_select_buf_line_width, 1, 1, orion_cfg_ptr_);
}

VCAllocator::~VCAllocator()
{
  delete m_local_arb_ptr;
  delete m_global_arb_ptr;
  delete m_vc_select_ptr;
}

double VCAllocator::get_dynamic_energy_local_vc_arb(double num_req_, bool is_max_) const
{
  double e_local_arb = 0;
  switch(m_va_model)
  {
    case TWO_STAGE_ARB:
      e_local_arb = m_local_arb_ptr->calc_dynamic_energy(num_req_, is_max_);
      break;
    case ONE_STAGE_ARB:
    case VC_SELECT:
    default:
      e_local_arb = 0;
  }
  return e_local_arb;
}

double VCAllocator::get_dynamic_energy_global_vc_arb(double num_req_, bool is_max_) const
{
  double e_global_arb = 0;
  switch(m_va_model)
  {
    case ONE_STAGE_ARB:
    case TWO_STAGE_ARB:
      e_global_arb = m_global_arb_ptr->calc_dynamic_energy(num_req_, is_max_);
      break;
    case VC_SELECT:
    default:
      e_global_arb = 0;
  }
  return e_global_arb;
}

double VCAllocator::get_dynamic_energy_vc_select(bool is_read_, bool is_max_) const
{
  double e_vc_select = 0;
  switch(m_va_model)
  {
    case VC_SELECT:
      e_vc_select = m_vc_select_ptr->get_dynamic_energy(is_read_, is_max_);
      break;
    case ONE_STAGE_ARB:
    case TWO_STAGE_ARB:
    default:
      e_vc_select = 0;
  }
  return e_vc_select;
}

double VCAllocator::get_static_power() const
{
  double p_va = 0;
  switch(m_va_model)
  {
    case ONE_STAGE_ARB:
      p_va = m_global_arb_ptr->get_static_power()*m_num_out_port*m_num_vclass*m_num_vchannel;
      break;
    case TWO_STAGE_ARB:
      p_va += m_local_arb_ptr->get_static_power()*m_num_in_port*m_num_vclass*m_num_vchannel;
      p_va += m_global_arb_ptr->get_static_power()*m_num_out_port*m_num_vclass*m_num_vchannel;
      break;   
    case VC_SELECT:
      p_va = m_vc_select_ptr->get_static_power()*m_num_out_port*m_num_vclass;
      break;
    default:
      cerr << "ERROR: Invalid VA model" << endl;
      exit(1);
  }
  return p_va;
}

VCAllocator* VCAllocator::create_vcallocator(
  const string& vcalloc_model_str_,
  uint32_t num_in_port_,
  uint32_t num_out_port_,
  uint32_t num_vclass_,
  uint32_t num_vchannel_,
  const OrionConfig* orion_cfg_ptr_
)
{
  uint32_t num_vchannel = orion_cfg_ptr_->get<uint32_t>("NUM_VIRTUAL_CHANNEL");

  if (num_vchannel > 1)
  {
    if (vcalloc_model_str_ == string("ONE_STAGE_ARB"))
    {
      const string& arb_model_str = orion_cfg_ptr_->get<string>("VA_OUT_ARB_MODEL");
      const string& arb_ff_model_str = orion_cfg_ptr_->get<string>("VA_OUT_ARB_FF_MODEL");
      const TechParameter* tech_param_ptr = orion_cfg_ptr_->get_tech_param_ptr();
      return new VCAllocator(num_in_port_, num_out_port_, num_vclass_, num_vchannel_, 
        arb_model_str, arb_ff_model_str, tech_param_ptr);
    }
    else if (vcalloc_model_str_ == string("TWO_STAGE_ARB"))
    {
      const string& local_arb_model_str = orion_cfg_ptr_->get<string>("VA_IN_ARB_MODEL");
      const string& local_arb_ff_model_str = orion_cfg_ptr_->get<string>("VA_IN_ARB_FF_MODEL");
      const string& global_arb_model_str = orion_cfg_ptr_->get<string>("VA_OUT_ARB_MODEL");
      const string& global_arb_ff_model_str = orion_cfg_ptr_->get<string>("VA_OUT_ARB_FF_MODEL");
      const TechParameter* tech_param_ptr = orion_cfg_ptr_->get_tech_param_ptr();
      return new VCAllocator(num_in_port_, num_out_port_, num_vclass_, num_vchannel_, 
        local_arb_model_str, local_arb_ff_model_str, 
        global_arb_model_str, global_arb_ff_model_str,tech_param_ptr);
    }
    else if (vcalloc_model_str_ == string("VC_SELECT"))
    {
      const string& vc_select_buf_model_str = orion_cfg_ptr_->get<string>("VA_BUF_MODEL");
      return new VCAllocator(num_in_port_, num_out_port_, num_vclass_, num_vchannel_,
        vc_select_buf_model_str, orion_cfg_ptr_);
    }
    else
    {
      cerr << "WARNING: No VC allocator model" << endl;
      return (VCAllocator*)NULL;
    }
  }
  else
  {
    return (VCAllocator*)NULL;
  }
}
