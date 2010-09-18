#ifndef __MEMUNIT_H__
#define __MEMUNIT_H__

#include "Type.h"

class SRAM;
class TechParameter;

class MemUnit
{
  public:
    enum MemModel
    {
      NO_MODEL = 0,
      NORMAL_MEM
    };

  public:
    MemUnit(
      const string& mem_model_str_,
      const SRAM* sram_ptr_,
      const TechParameter* tech_param_ptr_
    );
    ~MemUnit();

  public:
    double get_e_switch() const { return m_e_switch; }
    double get_i_static() const { return m_i_static; }

  private:
    void init();
    double calc_mem_cap();
    double calc_i_static();

  private:
    MemModel m_mem_model;
    const SRAM* m_sram_ptr;
    const TechParameter* m_tech_param_ptr;

    double m_e_switch;
    double m_i_static;
};

#endif

