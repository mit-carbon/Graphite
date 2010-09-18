#ifndef __ARBITER_H__
#define __ARBITER_H__

#include "Type.h"

class TechParameter;
class FlipFlop;

class Arbiter
{
  public:
    enum ArbiterModel
    {
      NO_MODEL = 0,
      RR_ARBITER,
      MATRIX_ARBITER
    };

  public:
    Arbiter(
      const ArbiterModel arb_model_,
      const uint32_t req_width_,
      const double len_in_wire_,
      const TechParameter* tech_param_ptr_
    );
    virtual ~Arbiter() = 0;

  public:
    virtual double calc_dynamic_energy(double num_req_, bool is_max_) const = 0;
    double get_static_power() const;

  protected:
    ArbiterModel m_arb_model;
    uint32_t m_req_width;
    double m_len_in_wire;
    const TechParameter* m_tech_param_ptr;

    FlipFlop* m_ff_ptr;

    double m_e_chg_req;
    double m_e_chg_grant;

    double m_i_static;

  public:
    static Arbiter* create_arbiter(
      const string& arb_model_str_,
      const string& ff_model_str_,
      uint32_t req_width_,
      double len_in_wire_,
      const TechParameter* tech_param_ptr_
    );
};

#endif

