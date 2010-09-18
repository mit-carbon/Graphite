#ifndef __REGISTER_H__
#define __REGISTER_H__

#include "Type.h"

class TechParameter;
class FlipFlop;

class Register
{
  public:
    Register(
      uint32_t num_entry_,
      uint32_t line_width_,
      const TechParameter* tech_param_ptr_
    );
    ~Register();

  public:
    double calc_e_read() const;
    double calc_e_write() const;
    double calc_i_static() const;

  private:
    void init();

  private:
    uint32_t m_num_entry;
    uint32_t m_line_width;
    const TechParameter* m_tech_param_ptr;

    FlipFlop* m_ff_ptr;

    double m_avg_read;
    double m_avg_write;
};

#endif
