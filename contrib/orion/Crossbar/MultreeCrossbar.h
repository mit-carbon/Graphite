#ifndef __MULTREECROSSBAR_H__
#define __MULTREECROSSBAR_H__

#include "Type.h"
#include "Crossbar.h"

class TechParameter;

class MultreeCrossbar : public Crossbar
{
  public:
    MultreeCrossbar(
      const string& conn_type_str_,
      const string& trans_type_str_,
      uint32_t num_in_,
      uint32_t num_out_,
      uint32_t data_width_,
      uint32_t degree_,
      const TechParameter* tech_param_ptr_
    );
    ~MultreeCrossbar();

  public:
    double get_dynamic_energy(bool is_max_) const;

  private:
    void init();
    double calc_i_static();

  private:
    uint32_t m_depth;
};

#endif
