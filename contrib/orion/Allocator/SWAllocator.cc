#include "SWAllocator.h"

#include <iostream>
#include <cassert>

#include "Util.h"
#include "OrionConfig.h"
#include "Arbiter.h"
#include "Crossbar.h"

using namespace std;

SWAllocator::SWAllocator(
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
)
{
  assert(num_in_port_ == num_in_port_);
  assert(num_out_port_ == num_out_port_);
  assert(num_vclass_ == num_vclass_);
  assert(num_vchannel_ == num_vchannel_);
  assert(len_in_wire_ == len_in_wire_);

  m_num_in_port = num_in_port_;
  m_num_out_port = num_out_port_;
  m_num_vclass = num_vclass_;
  m_num_vchannel = num_vchannel_;

  if ((m_num_vclass*m_num_vchannel) > 1)
  {
    m_local_arb_ptr = Arbiter::create_arbiter(
      local_arb_model_str_, local_arb_ff_model_str_,
      m_num_vclass*m_num_vchannel, 0, tech_param_ptr_);
  }
  else
  {
    m_local_arb_ptr = NULL;
  }

  if (m_num_in_port > 2)
  {
    m_global_arb_ptr = Arbiter::create_arbiter(
      global_arb_model_str_, global_arb_ff_model_str_,
      m_num_in_port-1, len_in_wire_, tech_param_ptr_);
  }
  else
  {
    m_global_arb_ptr = NULL;
  }
}

SWAllocator::~SWAllocator()
{}

double SWAllocator::get_dynamic_energy_local_sw_arb(double num_req_, bool is_max_) const
{
  double e_local_arb = 0;

  if (m_local_arb_ptr)
  {
    e_local_arb = m_local_arb_ptr->calc_dynamic_energy(num_req_, is_max_);
  }
  return e_local_arb;
}

double SWAllocator::get_dynamic_energy_global_sw_arb(double num_req_, bool is_max_) const
{
  double e_global_arb = 0;

  if (m_global_arb_ptr)
  {
    e_global_arb = m_global_arb_ptr->calc_dynamic_energy(num_req_, is_max_);
  }
  return e_global_arb;
}

double SWAllocator::get_static_power() const
{
  double p_va = 0;

  if (m_local_arb_ptr)
  {
    // FIXME: might not be m_num_in_port;
    p_va += m_local_arb_ptr->get_static_power()*m_num_in_port;
  }
  if (m_global_arb_ptr)
  {
    p_va += m_global_arb_ptr->get_static_power()*m_num_out_port;
  }
  return p_va;
}

void SWAllocator::print_all() const
{
  cout << "SWAllocator:" << endl;
  if (m_local_arb_ptr)
  {
    for (uint32_t i = 0; i < m_num_vclass*m_num_vchannel; i++)
    {
      cout << "\t" << "Local arb (" << i << ") = " << get_dynamic_energy_local_sw_arb(i, false) << endl;
    }
  }

  if (m_global_arb_ptr)
  {
    for (uint32_t i = 0; i < m_num_in_port-1; i++)
    {
      cout << "\t" << "Global arb (" << i << ") = " << get_dynamic_energy_global_sw_arb(i, false) << endl;
    }
  }

  cout << "\t" << "Static power = " << get_static_power() << endl;
  return;
}

SWAllocator* SWAllocator::create_swallocator(
  uint32_t num_in_port_, 
  uint32_t num_out_port_, 
  uint32_t num_vclass_, 
  uint32_t num_vchannel_, 
  const Crossbar* xbar_ptr_,
  const OrionConfig* orion_cfg_ptr_
)
{
  double len_in_wire = xbar_ptr_->get_len_req_wire();
  const string& local_arb_model_str = orion_cfg_ptr_->get<string>("SA_IN_ARB_MODEL");
  const string& local_arb_ff_model_str = orion_cfg_ptr_->get<string>("SA_IN_ARB_FF_MODEL");
  const string& global_arb_model_str = orion_cfg_ptr_->get<string>("SA_OUT_ARB_MODEL");
  const string& global_arb_ff_model_str = orion_cfg_ptr_->get<string>("SA_OUT_ARB_FF_MODEL");
  const TechParameter* tech_param_ptr = orion_cfg_ptr_->get_tech_param_ptr();
  return new SWAllocator(num_in_port_, num_out_port_, num_vclass_, num_vchannel_, 
    len_in_wire, local_arb_model_str, local_arb_ff_model_str, 
    global_arb_model_str, global_arb_ff_model_str,tech_param_ptr);
}
