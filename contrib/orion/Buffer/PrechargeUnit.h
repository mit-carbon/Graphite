#ifndef __PRECHARGEUNIT_H__
#define __PRECHARGEUNIT_H__

#include "Type.h"

class SRAM;
class TechParameter;

class PrechargeUnit
{
  public:
    enum PrechargeModel
    {
      NO_MODEL = 0,
      SINGLE_BITLINE,
      EQU_BITLINE,
      SINGLE_OTHER
    };

  public:
    PrechargeUnit(
      const string& pre_model_str_,
      double pre_load_,
      const SRAM* sram_ptr_,
      const TechParameter* tech_param_ptr_
    );
    ~PrechargeUnit();

  public:
    double get_e_charge_gate() const { return m_e_charge_gate; }
    double get_e_charge_drain() const { return m_e_charge_drain; }
    double get_i_static() const { return m_i_static; }

  private:
    void init();
    uint32_t calc_num_pre_gate();
    uint32_t calc_num_pre_drain();
    double calc_pre_cap(double width_, double length_);

  private:
    PrechargeModel m_pre_model;
    double m_pre_load;
    const SRAM* m_sram_ptr;
    const TechParameter* m_tech_param_ptr;

    double m_pre_size;

    double m_e_charge_gate;
    double m_e_charge_drain;

    double m_i_static;
};

#endif

