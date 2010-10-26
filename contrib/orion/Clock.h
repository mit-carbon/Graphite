#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "Type.h"

class TechParameter;
class OrionConfig;

class Clock
{
  public:
    Clock(
      bool is_in_buf_,
      bool is_in_shared_switch_,
      bool is_out_buf_,
      bool is_out_shared_switch_,
      const OrionConfig* orion_cfg_ptr_
    );
    ~Clock();

  public:
    double get_dynamic_energy() const;
    double get_static_power() const;

  private:
    void init();
    double calc_cap_pipe_reg();

  private:
    bool m_is_in_buf;
    bool m_is_in_shared_switch;
    bool m_is_out_buf;
    bool m_is_out_shared_switch;
    const OrionConfig* m_orion_cfg_ptr;
    const TechParameter* m_tech_param_ptr;

    double m_e_pipe_reg;
    double m_e_htree;
    double m_i_static;
};

#endif
