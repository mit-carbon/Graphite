#ifndef __CROSSBAR_H__
#define __CROSSBAR_H__

#include "Type.h"

class TechParameter;
class OrionConfig;

class Crossbar
{
  public:
    enum CrossbarModel
    {
      NO_MODEL = 0,
      MATRIX_CROSSBAR,
      MULTREE_CROSSBAR
    };
    enum ConnectType
    {
      TRANS_GATE,
      TRISTATE_GATE
    };
    enum TransType
    {
      N_GATE,
      NP_GATE
    };

  public:
    Crossbar(
      CrossbarModel xbar_model_,
      const string& conn_type_str_,
      const string& trans_type_str_,
      uint32_t num_in_,
      uint32_t num_out_,
      uint32_t data_width_,
      uint32_t num_in_seg_,
      uint32_t num_out_seg_,
      uint32_t degree_,
      const TechParameter* tech_param_ptr_
    );
    virtual ~Crossbar() = 0;

  public:
    double get_len_req_wire() const { return m_len_req_wire; }

    virtual double get_dynamic_energy(bool is_max_) const = 0;
    double get_static_power() const;

  protected:
    void set_conn_type(const string& conn_type_str_);
    void set_trans_type(const string& trans_type_str_);
    double calc_in_cap();
    double calc_out_cap(uint32_t num_in_);
    double calc_int_cap();
    double calc_ctr_cap(double cap_wire_, bool prev_ctr_, bool next_ctr_);
    virtual double calc_i_static() = 0;

  protected:
    CrossbarModel m_xbar_model;
    ConnectType m_conn_type;
    TransType m_trans_type;
    uint32_t m_num_in;
    uint32_t m_num_out;
    uint32_t m_data_width;
    uint32_t m_num_in_seg;
    uint32_t m_num_out_seg;
    uint32_t m_degree;
    const TechParameter* m_tech_param_ptr;

    double m_cap_in_wire;
    double m_cap_out_wire;
    double m_cap_ctr_wire;
    double m_len_req_wire;

    double m_e_chg_in;
    double m_e_chg_out;
    double m_e_chg_ctr;
    double m_e_chg_int;

    double m_i_static;

  public:
    static Crossbar* create_crossbar(
      const string& xbar_model_str_,
      uint32_t num_in_,
      uint32_t num_out_,
      uint32_t data_width_,
      const OrionConfig* orion_cfg_ptr_
    );
};

#endif
