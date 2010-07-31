#ifndef __DECODERUNIT_H__
#define __DECODERUNIT_H__

#include "Type.h"
#include "TechParameter.h"

class DecoderUnit
{
  public:
    enum DecModel
    {
      NO_MODEL = 0,
      GENERIC_DEC
    };

  public:
    DecoderUnit(
      const string& dec_model_str_, 
      uint32_t dec_width_, 
      const TechParameter* tech_param_ptr_
    );
    ~DecoderUnit();

  public:
    uint32_t get_dec_width() const { return m_dec_width; }
    uint32_t get_num_in_2nd() const { return m_num_in_2nd; }
    double get_e_chg_addr() const { return m_e_chg_addr; }
    double get_e_chg_output() const { return m_e_chg_output; }
    double get_e_chg_l1() const { return m_e_chg_l1; }

  private:
    void init();
    double calc_chgl1_cap();
    double calc_select_cap();
    double calc_chgaddr_cap();

  private:
    DecModel m_dec_model;
    uint32_t m_dec_width;
    const TechParameter* m_tech_param_ptr;

    uint32_t m_num_in_1st;
    uint32_t m_num_in_2nd;
    uint32_t m_num_out_0th;
    uint32_t m_num_out_1st;

    double m_e_chg_l1;
    double m_e_chg_output;
    double m_e_chg_addr;
};

#endif

