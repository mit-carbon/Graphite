#ifndef __MATRIXARBITER_H__
#define __MATRIXARBITER_H__

#include "Type.h"

#include "Arbiter.h"

class TechParameter;

class MatrixArbiter : public Arbiter
{
  public:
    MatrixArbiter(
      const string& ff_model_str_,
      uint32_t req_width_,
      double len_in_wire_,
      const TechParameter* tech_param_ptr_
    );
    ~MatrixArbiter();

  public:
    double calc_dynamic_energy(double num_req_, bool is_max_) const;

  private:
    void init(const string& ff_model_str_);
    double calc_req_cap();
    double calc_pri_cap();
    double calc_grant_cap();
    double calc_int_cap();
    double calc_i_static();

  private:
    double m_e_chg_int;
};

#endif
