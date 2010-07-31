#ifndef __MATRIXCROSSBAR_H__
#define __MATRIXCROSSBAR_H__

#include "Type.h"

#include "Crossbar.h"

class TechParameter;

class MatrixCrossbar : public Crossbar
{
  public:
    MatrixCrossbar(
      const string& conn_type_str_,
      const string& trans_type_str_,
      uint32_t num_in_,
      uint32_t num_out_,
      uint32_t data_width_,
      uint32_t num_in_seg_,
      uint32_t num_out_seg_,
      double len_in_wire_,
      double len_out_wire_,
      const TechParameter* tech_param_ptr_
    );
    ~MatrixCrossbar();

  public:
    double get_dynamic_energy(bool is_max_) const;

  private:
    void init();
    double calc_i_static();

  private:
    double m_len_in_wire;
    double m_len_out_wire;
};

#endif
