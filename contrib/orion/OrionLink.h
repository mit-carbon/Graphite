#ifndef __ORIONLINK_H__
#define __ORIONLINK_H__

#include "Type.h"

class TechParameter;
class OrionConfig;

class OrionLink
{
  public:
    OrionLink(
      float frequency_,
      double len_,
      uint32_t line_width_,
      OrionConfig* orion_cfg_ptr_
    );
    ~OrionLink();

  public:
    double calc_dynamic_energy(uint32_t num_bit_flip_) const;
    double get_static_power() const;

    void print() const;

  private:
    void init();

  private:
    double m_len;
    uint32_t m_line_width;
    const OrionConfig* m_orion_cfg_ptr;

    double m_dynamic_energy_per_bit;
    double m_static_power_per_bit;
};

#endif
