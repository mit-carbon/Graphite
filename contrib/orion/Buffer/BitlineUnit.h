#ifndef __BITLINEUNIT_H__
#define __BITLINEUNIT_H__

#include "Type.h"

class SRAM;
class TechParameter;

class BitlineUnit
{
  public:
    enum BitlineModel
    {
      NO_MODEL = 0,
      RW_BITLINE,
      WO_BITLINE
    };

  public:
    BitlineUnit(
      const string bl_model_str_,
      const SRAM* sram_ptr_,
      const TechParameter* tech_param_ptr_
    );
    ~BitlineUnit();

  public:
    double get_pre_unit_load() const { return m_pre_unit_load; }

    double get_e_col_read() const { return m_e_col_read; }
    double get_e_col_wrtie() const { return m_e_col_write; }
    double get_i_static() const { return m_i_static; }

  private:
    void init();
    double calc_col_select_cap();
    double calc_col_read_cap();
    double calc_col_write_cap();
    double calc_i_static();

  private:
    BitlineModel m_bl_model;
    const SRAM* m_sram_ptr;
    const TechParameter* m_tech_param_ptr;

    double m_bl_len;
    double m_bl_wire_cap;
    double m_pre_unit_load;

    double m_e_col_sel;
    double m_e_col_read;
    double m_e_col_write;

    double m_i_static;
};

#endif

