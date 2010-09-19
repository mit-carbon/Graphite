#ifndef __RRARBITER_H__
#define __RRARBITER_H__

#include "Type.h"
#include "Arbiter.h"

class TechParameter;

class RRArbiter : public Arbiter
{
  public:
    RRArbiter(
      const string& ff_model_str_,
      uint32_t req_width_,
      double len_in_wire_,
      const TechParameter* tech_param_ptr_
    );
    ~RRArbiter();

  public:
    double calc_dynamic_energy(double num_req_, bool is_max_) const;

  private:
    void init(const string& ff_model_str_);
    double calc_req_cap();
    double calc_pri_cap();
    double calc_grant_cap();
    double calc_carry_cap();
    double calc_carry_in_cap();
    double calc_i_static();

  private:
    double m_e_chg_carry;
    double m_e_chg_carry_in;
};

#endif
