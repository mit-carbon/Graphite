#include "OrionRouter.h"

#include <cassert>

#include "Util.h"
#include "OrionConfig.h"
#include "Buffer.h"
#include "Crossbar.h"
#include "VCAllocator.h"
#include "SWAllocator.h"
#include "Clock.h"

OrionRouter::OrionRouter(
  const OrionConfig* orion_cfg_ptr_
)
{
  m_orion_cfg_ptr = orion_cfg_ptr_;

  init();
}

OrionRouter::OrionRouter(
  float frequency_,
  uint32_t num_in_port_,
  uint32_t num_out_port_,
  uint32_t num_vclass_,
  uint32_t num_vchannel_,
  uint32_t in_buf_num_set_,
  uint32_t flit_width_,
  OrionConfig* orion_cfg_ptr_
)
{
  assert(frequency_ > 0);
  assert((num_in_port_ == num_in_port_) && (num_in_port_ != 0));
  assert((num_out_port_ == num_out_port_) && (num_out_port_ != 0));
  assert((num_vclass_ == num_vclass_) && (num_vclass_ != 0));
  assert((num_vchannel_ == num_vchannel_) && (num_vchannel_ != 0));
  assert(in_buf_num_set_ == in_buf_num_set_);
  assert((flit_width_ == flit_width_) && (flit_width_ != 0));

  orion_cfg_ptr_->set_frequency(frequency_);
  orion_cfg_ptr_->set_num_in_port(num_in_port_);
  orion_cfg_ptr_->set_num_out_port(num_out_port_);
  orion_cfg_ptr_->set_num_vclass(num_vclass_);
  orion_cfg_ptr_->set_num_vchannel(num_vchannel_);
  orion_cfg_ptr_->set_in_buf_num_set(in_buf_num_set_);
  orion_cfg_ptr_->set_flit_width(flit_width_);
  m_orion_cfg_ptr = orion_cfg_ptr_;

  init();
}

OrionRouter::~OrionRouter()
{
  delete m_in_buf_ptr;
  delete m_xbar_ptr;
  delete m_va_ptr;
  delete m_sa_ptr;
  delete m_clk_ptr;
}

//double OrionRouter::calc_dynamic_energy(double e_fin_) const
//{
//  double dynamic_e_buf = 0, static_p_buf = 0;
//  double dynamic_e_xbar = 0, static_p_xbar = 0;
//  double dynamic_e_va = 0, static_p_va = 0;
//  double dynamic_e_sa = 0, static_p_sa = 0;
//  double dynamic_e_clock = 0, static_p_clock = 0;
//
//  // input buffer
//  double e_in_buf_rw = e_fin_*m_num_in_port;
//  dynamic_e_buf += calc_dynamic_energy_buf(true)*e_in_buf_rw;
//  dynamic_e_buf += calc_dynamic_energy_buf(false)*e_in_buf_rw;
//  static_p_buf = get_static_power_buf();
//  
//  // output buffer
//  double e_out_buf_rw = e_fin_*m_num_in_port;
//  dynamic_e_buf += calc_dynami
//
//}

double OrionRouter::calc_dynamic_energy_buf(bool is_read_, bool is_max_) const
{
  if (m_in_buf_ptr)
  {
    return m_in_buf_ptr->get_dynamic_energy(is_read_, is_max_);
  }
  else
  {
    return 0;
  }
}

//double OrionRouter::calc_dynamic_energy_out_buf(bool is_read_, bool is_max_) const
//{
//  if (m_out_buf_ptr)
//  {
//    return m_out_buf_ptr->get_dynamic_energy(is_read_, is_max_);
//  }
//  else
//  {
//    return 0;
//  }
//}

double OrionRouter::calc_dynamic_energy_xbar(bool is_max_) const
{
  if (m_xbar_ptr)
  {
    return m_xbar_ptr->get_dynamic_energy(is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_local_vc_arb(double num_req_, bool is_max_) const
{
  if (m_va_ptr)
  {
    return m_va_ptr->get_dynamic_energy_local_vc_arb(num_req_, is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_global_vc_arb(double num_req_, bool is_max_) const
{
  if (m_va_ptr)
  {
    return m_va_ptr->get_dynamic_energy_global_vc_arb(num_req_, is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_vc_select(bool is_read_, bool is_max_) const
{
  if (m_va_ptr)
  {
    return m_va_ptr->get_dynamic_energy_vc_select(is_read_, is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_local_sw_arb(double num_req_, bool is_max_) const
{
  if (m_sa_ptr)
  {
    return m_sa_ptr->get_dynamic_energy_local_sw_arb(num_req_, is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_global_sw_arb(double num_req_, bool is_max_) const
{
  if (m_sa_ptr)
  {
    return m_sa_ptr->get_dynamic_energy_global_sw_arb(num_req_, is_max_);
  }
  else
  {
    return 0;
  }
}

double OrionRouter::calc_dynamic_energy_clock() const
{
  if (m_clk_ptr)
  {
    return m_clk_ptr->get_dynamic_energy();
  }
  else
  {
    return 0;
  }
}

double OrionRouter::get_static_power_buf() const
{
  if (m_in_buf_ptr)
  {
    uint32_t num_in_buf;
    if (m_is_in_shared_buf)
    {
      num_in_buf = m_num_vclass*m_num_in_port;
    }
    else
    {
      num_in_buf = m_num_vchannel*m_num_vclass*m_num_in_port;
    }
    return m_in_buf_ptr->get_static_power()*(double)num_in_buf;
  }
  else
  {
    return 0;
  }
}

double OrionRouter::get_static_power_xbar() const
{
  if (m_xbar_ptr)
  {
    return m_xbar_ptr->get_static_power();
  }
  else
  {
    return 0;
  }
}

double OrionRouter::get_static_power_va() const
{
  if (m_va_ptr)
  {
    return m_va_ptr->get_static_power();
  }
  else
  {
    return 0;
  }
}

double OrionRouter::get_static_power_sa() const
{
  if (m_sa_ptr)
  {
    return m_sa_ptr->get_static_power();
  }
  else
  {
    return 0;
  }
}

double OrionRouter::get_static_power_clock() const
{
  if (m_clk_ptr)
  {
    return m_clk_ptr->get_static_power();
  }
  else
  {
    return 0;
  }
}

void OrionRouter::init()
{
  m_num_in_port = m_orion_cfg_ptr->get<uint32_t>("NUM_INPUT_PORT");
  m_num_out_port = m_orion_cfg_ptr->get<uint32_t>("NUM_OUTPUT_PORT");
  m_flit_width = m_orion_cfg_ptr->get<uint32_t>("FLIT_WIDTH");
  m_num_vclass = m_orion_cfg_ptr->get<uint32_t>("NUM_VIRTUAL_CLASS");
  m_num_vchannel = m_orion_cfg_ptr->get<uint32_t>("NUM_VIRTUAL_CHANNEL");

  if ((m_num_vclass*m_num_vchannel) > 1)
  {
    m_is_in_shared_buf = m_orion_cfg_ptr->get<bool>("IS_IN_SHARED_BUFFER");
    m_is_out_shared_buf = m_orion_cfg_ptr->get<bool>("IS_OUT_SHARED_BUFFER");
    m_is_in_shared_switch = m_orion_cfg_ptr->get<bool>("IS_IN_SHARED_SWITCH");
    m_is_out_shared_switch = m_orion_cfg_ptr->get<bool>("IS_OUT_SHARED_SWITCH");
  }
  else
  {
    m_is_in_shared_buf = false;
    m_is_out_shared_buf = false;
    m_is_in_shared_switch = false;
    m_is_out_shared_switch = false;
  }

  //input buffer
  bool is_in_buf = m_orion_cfg_ptr->get<bool>("IS_INPUT_BUFFER");
  if (is_in_buf)
  {
    bool is_fifo = true;
    bool is_outdrv = (!m_is_in_shared_buf) && (m_is_in_shared_switch);
    const string& in_buf_model_str = m_orion_cfg_ptr->get<string>("IN_BUF_MODEL");
    uint32_t in_buf_num_set = m_orion_cfg_ptr->get<uint32_t>("IN_BUF_NUM_SET");
    uint32_t in_buf_num_read_port = m_orion_cfg_ptr->get<uint32_t>("IN_BUF_NUM_READ_PORT");
    m_in_buf_ptr = new Buffer(in_buf_model_str, is_fifo, is_outdrv, 
      in_buf_num_set, m_flit_width, in_buf_num_read_port, 1, m_orion_cfg_ptr);
  }
  else
  {
    m_in_buf_ptr = NULL;
  }

  //output buffer
  bool is_out_buf = m_orion_cfg_ptr->get<bool>("IS_OUTPUT_BUFFER");

  //crossbar
  uint32_t num_switch_in;
  if (is_in_buf)
  {
    if (m_is_in_shared_buf)
    {
      uint32_t in_buf_num_read_port = m_orion_cfg_ptr->get<uint32_t>("IN_BUF_NUM_READ_PORT");
      num_switch_in = in_buf_num_read_port*m_num_in_port;
    }
    else if (m_is_in_shared_switch)
    {
      num_switch_in = 1*m_num_in_port;
    }
    else
    {
      num_switch_in = m_num_vclass*m_num_vchannel*m_num_in_port;
    }
  }
  else
  {
    num_switch_in = 1*m_num_in_port;
  }
  uint32_t num_switch_out;
  if (is_out_buf)
  {
    if (m_is_out_shared_buf)
    {
      uint32_t out_buf_num_write_port = m_orion_cfg_ptr->get<uint32_t>("OUT_BUF_NUM_WRITE_PORT");
      num_switch_out = out_buf_num_write_port*m_num_out_port;
    }
    else if (m_is_out_shared_switch)
    {
      num_switch_out = 1*m_num_out_port;
    }
    else
    {
      num_switch_out = m_num_vchannel*m_num_vclass*m_num_out_port;
    }
  }
  else
  {
    num_switch_out = 1*m_num_out_port;
  }
  const string& xbar_model_str = m_orion_cfg_ptr->get<string>("CROSSBAR_MODEL");
  m_xbar_ptr = Crossbar::create_crossbar(xbar_model_str, 
    num_switch_in, num_switch_out, m_flit_width, m_orion_cfg_ptr);

  //vc allocator
  const string& va_model_str = m_orion_cfg_ptr->get<string>("VA_MODEL");
  m_va_ptr = VCAllocator::create_vcallocator(va_model_str,
    m_num_in_port, m_num_out_port, m_num_vclass, m_num_vchannel, m_orion_cfg_ptr);

  //sw allocator
  m_sa_ptr = SWAllocator::create_swallocator(
    m_num_in_port, m_num_out_port, m_num_vclass, m_num_vchannel, 
    m_xbar_ptr, m_orion_cfg_ptr);

  //cloc
  m_clk_ptr = new Clock(is_in_buf, m_is_in_shared_switch, is_out_buf, m_is_out_shared_switch, m_orion_cfg_ptr);

  return;
}

void OrionRouter::print() const
{
  cout << "Router - Dynamic Energy" << endl;
  cout << "\t" << "Buffer Read = " << calc_dynamic_energy_buf(true) << endl;
  cout << "\t" << "Buffer Write = " << calc_dynamic_energy_buf(false) << endl;
  cout << "\t" << "Crossbar = " << calc_dynamic_energy_xbar() << endl;
  cout << "\t" << "Local VC Allocator(1) = " << calc_dynamic_energy_local_vc_arb(1) << endl;
  cout << "\t" << "Global VC Allocator(1) = " << calc_dynamic_energy_global_vc_arb(1) << endl;
  cout << "\t" << "VC Select Read = " << calc_dynamic_energy_vc_select(true) << endl;
  cout << "\t" << "VC Select Write = " << calc_dynamic_energy_vc_select(false) << endl;
  cout << "\t" << "Local SW Allocator(1) = " << calc_dynamic_energy_local_sw_arb(1) << endl;
  cout << "\t" << "Global SW Allocator(1) = " << calc_dynamic_energy_global_sw_arb(1) << endl;
  cout << "\t" << "Clock = " << calc_dynamic_energy_clock() << endl;
  cout << endl;
  cout << "Router - Static Power" << endl;
  cout << "\t" << "Buffer = " << get_static_power_buf() << endl;
  cout << "\t" << "Crossbar = " << get_static_power_xbar() << endl;
  cout << "\t" << "VC Allocator = " << get_static_power_va() << endl;
  cout << "\t" << "SW Allocator = " << get_static_power_sa() << endl;
  cout << "\t" << "Clock = " << get_static_power_clock() << endl;
  cout << endl;
  return;
}

