#ifndef __WIRE_H__
#define __WIRE_H__

#include "Type.h"

class TechParameter;

class Wire
{
  public:
    enum WidthSpacingModel
    {
      SWIDTH_SSPACE,
      SWIDTH_DSPACE,
      DWIDTH_SSPACE,
      DWIDTH_DSPACE
    };
    enum BufferScheme
    {
      MIN_DELAY,
      STAGGERED
    };

  public:
    Wire(
      const string& wire_spacing_model_str_,
      const string& buf_scheme_str_,
      bool is_shielding_,
      const TechParameter* tech_param_ptr_
    );
    ~Wire();

  public:
    void calc_opt_buffering(int* k_, double* h_, double len_) const;
    double calc_dynamic_energy(double len_) const;
    double calc_static_power(double len_) const;

  private:
    void init();
    void set_width_spacing_model(const string& wire_spacing_model_str_);
    void set_buf_scheme(const string& buf_scheme_str_);
    double calc_res_unit_len();
    double calc_gnd_cap_unit_len();
    double calc_couple_cap_unit_len();

  private:
    const TechParameter* m_tech_param_ptr;
    WidthSpacingModel m_width_spacing_model;
    BufferScheme m_buf_scheme;
    bool m_is_shielding;

    double m_res_unit_len;
    double m_gnd_cap_unit_len;
    double m_couple_cap_unit_len;
};

#endif
