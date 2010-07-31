#include "Buffer.h"

#include <iostream>

#include "TechParameter.h"
#include "OrionConfig.h"
#include "SRAM.h"
#include "Register.h"

using namespace std;

Buffer::Buffer(
  const string& buffer_model_str_,
  bool is_fifo_,
  bool is_outdrv_,
  uint32_t num_entry_,
  uint32_t line_width_,
  uint32_t num_read_port_,
  uint32_t num_write_port_,
  const OrionConfig* orion_cfg_ptr_
)
{
  if (buffer_model_str_ == string("SRAM"))
  {
    m_buffer_model = BUF_SRAM;
  }
  else if(buffer_model_str_ == string("REGISTER"))
  {
    m_buffer_model = BUF_REG;
  }
  else
  {
    m_buffer_model = NO_MODEL;
  }

  if (m_buffer_model != NO_MODEL)
  {
    m_num_entry = num_entry_;
    m_line_width = line_width_;
    m_is_fifo = is_fifo_;
    m_is_outdrv = is_outdrv_;
    m_num_read_port = num_read_port_;
    m_num_write_port = num_write_port_;

    m_orion_cfg_ptr = orion_cfg_ptr_;
    m_tech_param_ptr = m_orion_cfg_ptr->get_tech_param_ptr();

    init();
  }
}

Buffer::~Buffer()
{
  delete m_sram_ptr;
}

double Buffer::get_dynamic_energy(
  bool is_read_,
  bool is_max_
) const
{
  if (m_buffer_model == BUF_SRAM)
  {
    if (is_read_)
    {
      return m_sram_ptr->calc_e_read(is_max_);
    }
    else
    {
      return m_sram_ptr->calc_e_write(is_max_);
    }
  }
  else if (m_buffer_model == BUF_REG)
  {
    if (is_read_)
    {
      return m_reg_ptr->calc_e_read();
    }
    else
    {
      return m_reg_ptr->calc_e_write();
    }
  }
  else
  {
    return 0;
  }
}

double Buffer::get_static_power() const
{
  double vdd = m_tech_param_ptr->get_vdd();
  double SCALE_S = m_tech_param_ptr->get_SCALE_S();
  if (m_buffer_model == BUF_SRAM)
  {
    return m_sram_ptr->calc_i_static()*vdd*SCALE_S;
  }
  else if (m_buffer_model == BUF_REG)
  {
    return m_reg_ptr->calc_i_static()*vdd*SCALE_S;
  }
  else
  {
    return 0;
  }
}

void Buffer::init()
{
  if(m_buffer_model == BUF_SRAM)
  {
    uint32_t num_data_end = m_orion_cfg_ptr->get<uint32_t>("SRAM_NUM_DATA_END");
    const string& rowdec_model_str = m_orion_cfg_ptr->get<string>("SRAM_ROWDEC_MODEL");
    const string& wl_model_str = m_orion_cfg_ptr->get<string>("SRAM_WORDLINE_MODEL");
    const string& bl_pre_model_str = m_orion_cfg_ptr->get<string>("SRAM_BITLINE_PRE_MODEL");
    const string& mem_model_str = "NORMAL_MEM";
    const string& bl_model_str = m_orion_cfg_ptr->get<string>("SRAM_BITLINE_MODEL");
    const string& amp_model_str = m_orion_cfg_ptr->get<string>("SRAM_AMP_MODEL");
    const string& outdrv_model_str = m_orion_cfg_ptr->get<string>("SRAM_OUTDRV_MODEL");
    m_sram_ptr = new SRAM(
      m_num_entry, m_line_width, m_is_fifo, m_is_outdrv, 
      m_num_read_port, m_num_write_port, num_data_end, 
      rowdec_model_str, wl_model_str, bl_pre_model_str, mem_model_str, 
      bl_model_str, amp_model_str, outdrv_model_str, m_tech_param_ptr);
  }
  else if (m_buffer_model == BUF_REG)
  {
    m_sram_ptr = NULL;
    m_reg_ptr = new Register(m_num_entry, m_line_width, m_tech_param_ptr);
  }
  else
  {
    m_sram_ptr = NULL;
    m_reg_ptr = NULL;
  }
  return;
}

