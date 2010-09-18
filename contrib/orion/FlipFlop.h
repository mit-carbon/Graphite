#ifndef __FLIPFLOP_H__
#define __FLIPFLOP_H__

#include "Type.h"

class TechParameter;

class FlipFlop
{
  public:
    enum FFModel
    {
      NO_MODEL = 0,
      NEG_DFF
    };

  public:
    FlipFlop(
      const string& ff_model_str_,
      double load_,
      const TechParameter* tech_param_ptr_
    );
    ~FlipFlop();

  public:
    double get_e_switch() const { return m_e_switch; }
    double get_e_keep_1() const { return m_e_keep_1; }
    double get_e_keep_0() const { return m_e_keep_0; }
    double get_e_clock() const { return m_e_clock; }
    double get_i_static() const { return m_i_static; }

  private:
    void init();
    double calc_node_cap(uint32_t num_fanin_, uint32_t num_fanout_);
    double calc_clock_cap();
    double calc_i_static();

  private:
    FFModel m_ff_model;
    const TechParameter* m_tech_param_ptr;

    double m_load;

    double m_e_switch;
    double m_e_keep_1;
    double m_e_keep_0;
    double m_e_clock;

    double m_i_static;
};

#endif
