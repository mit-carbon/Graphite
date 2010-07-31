#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "Type.h"

class OrionConfig;
class TechParameter;

class SRAM;
class Register;

class Buffer
{
  public:
    enum BufferModel
    {
      NO_MODEL = 0,
      BUF_SRAM,
      BUF_REG
    };

  public:
    Buffer(
      const string& buffer_model_str_,
      bool is_fifo_,
      bool is_outdrv_,
      uint32_t num_entry_,
      uint32_t line_width_,
      uint32_t num_read_port_,
      uint32_t num_write_port_,
      const OrionConfig* orion_cfg_ptr_
    );
    ~Buffer();

  public:
    double get_dynamic_energy(bool is_read_, bool is_max_) const;
    double get_static_power() const;

  private:
    void init();

  private:
    BufferModel m_buffer_model;
    uint32_t m_num_entry;
    uint32_t m_line_width;

    bool m_is_fifo;
    bool m_is_outdrv;
    uint32_t m_num_read_port;
    uint32_t m_num_write_port;
    const OrionConfig* m_orion_cfg_ptr;
    const TechParameter* m_tech_param_ptr;

    SRAM* m_sram_ptr;
    Register* m_reg_ptr;
};

#endif

