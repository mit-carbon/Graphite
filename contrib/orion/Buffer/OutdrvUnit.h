#ifndef __OUTDRVUNIT_H__
#define __OUTDRVUNIT_H__

#include "Type.h"

class SRAM;
class TechParameter;

class OutdrvUnit
{
  public:
    enum OutdrvModel
    {
      NO_MODEL = 0,
      CACHE_OUTDRV,
      REG_OUTDRV
    };

  public:
    OutdrvUnit(
      const string& outdrv_model_str_,
      const SRAM* sram_ptr_,
      const TechParameter* tech_param_ptr_
    );
    ~OutdrvUnit();

  public:
    double get_e_select() const { return m_e_select; }
    double get_e_chg_data() const { return m_e_chg_data; }
    double get_e_out_0() const { return m_e_out_0; }
    double get_e_out_1() const { return m_e_out_1; }

  private:
    void init();
    double calc_select_cap();
    double calc_chgdata_cap();
    double calc_outdata_cap(bool value_);
    double calc_i_static();

  private:
    OutdrvModel m_outdrv_model;
    const SRAM* m_sram_ptr;
    const TechParameter* m_tech_param_ptr;

    double m_e_select;
    double m_e_out_1;
    double m_e_out_0;
    double m_e_chg_data;

    double m_i_static;
};

#endif

