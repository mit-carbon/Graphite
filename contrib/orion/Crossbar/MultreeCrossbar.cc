#include "MultreeCrossbar.h"

#include <iostream>
#include <cmath>

#include "TechParameter.h"

using namespace std;

MultreeCrossbar::MultreeCrossbar(
  const string& conn_type_str_, 
  const string& trans_type_str_, 
  uint32_t num_in_, 
  uint32_t num_out_, 
  uint32_t data_width_, 
  uint32_t degree_,
  const TechParameter *tech_param_ptr_
) : Crossbar(
      MULTREE_CROSSBAR, conn_type_str_, trans_type_str_,
      num_in_, num_out_, data_width_, 0, 0, degree_, tech_param_ptr_)
{
  m_len_req_wire = 0;
  init();
}

MultreeCrossbar::~MultreeCrossbar()
{}

double MultreeCrossbar::get_dynamic_energy(bool is_max_) const
{
  double e_atomic;
  double e_access = 0;

  e_atomic = m_e_chg_in*m_data_width*(is_max_? 1:0.5);
  e_access += e_atomic;

  e_atomic = m_e_chg_out*m_data_width*(is_max_? 1:0.5);
  e_access += e_atomic;

  e_atomic = m_e_chg_ctr;
  e_access += e_atomic;

  if (m_depth > 1)
  {
    e_atomic = m_e_chg_int*m_data_width*(m_depth-1)*(is_max_? 1:0.5);
    e_access += e_atomic;
  }

  return e_access;
}

void MultreeCrossbar::init()
{
  double CrsbarCellWidth = m_tech_param_ptr->get_CrsbarCellWidth();
  double CCmetal = m_tech_param_ptr->get_CCmetal();
  double Lamda = m_tech_param_ptr->get_Lamda();
  double CC3metal = m_tech_param_ptr->get_CC3metal();

  double len_in_wire;
  // input wire horizontal segment length
  len_in_wire = m_num_in*m_data_width*CrsbarCellWidth*(m_num_out/2);
  m_cap_in_wire = len_in_wire*CCmetal;
  // input wire vertical segment length
  len_in_wire = m_num_in*m_data_width*(5*Lamda)*(m_num_out/2);
  m_cap_in_wire += len_in_wire*CC3metal;
  m_cap_out_wire = 0;

  double Cmetal = m_tech_param_ptr->get_Cmetal();
  double len_ctr_wire = m_num_in*m_data_width*CrsbarCellWidth*(m_num_out/2)/2;
  m_cap_ctr_wire = Cmetal*len_ctr_wire;

  double e_factor = m_tech_param_ptr->get_EnergyFactor();
  m_e_chg_in = calc_in_cap()*e_factor;
  m_e_chg_out = calc_out_cap(m_degree)*e_factor;
  m_e_chg_int = calc_int_cap()*e_factor;

  m_depth = (uint32_t)ceil(log((double)m_num_in)/log((double)m_degree));

  // control signal should reset after transmission is done
  if (m_depth == 1)
  {
    // only one level of control sigal
    m_e_chg_ctr = calc_ctr_cap(m_cap_ctr_wire, 0, 0)*e_factor;
  }
  else
  {
    // first level and last level control signals
    m_e_chg_ctr = calc_ctr_cap(m_cap_ctr_wire, 0, 1)*e_factor + calc_ctr_cap(0, 1, 0)*e_factor;
    // intermediate control signals
    if (m_depth > 2)
    {
      m_e_chg_ctr += (m_depth-2)*calc_ctr_cap(0, 1, 1)*e_factor;
    }
  }

  m_i_static = calc_i_static();
}

double MultreeCrossbar::calc_i_static()
{
  double Wdecinvn = m_tech_param_ptr->get_Wdecinvn();
  double Wdecinvp = m_tech_param_ptr->get_Wdecinvp();
  double Woutdrivern = m_tech_param_ptr->get_Woutdrivern();
  double Woutdriverp = m_tech_param_ptr->get_Woutdriverp();
  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  double NOR2_TAB_0 = m_tech_param_ptr->get_NOR2_TAB(0);
  double NOR2_TAB_1 = m_tech_param_ptr->get_NOR2_TAB(1);
  double NOR2_TAB_2 = m_tech_param_ptr->get_NOR2_TAB(2);
  double NOR2_TAB_3 = m_tech_param_ptr->get_NOR2_TAB(3);
  double NMOS_TAB_0 = m_tech_param_ptr->get_NMOS_TAB(0);
  double PMOS_TAB_0 = m_tech_param_ptr->get_PMOS_TAB(0);

  double i_static = 0;

  // input driver
  i_static += (Wdecinvn*NMOS_TAB_0+Wdecinvp*PMOS_TAB_0)*m_num_in*m_data_width;

  // output driver
  i_static += (Woutdrivern*NMOS_TAB_0+Woutdriverp*PMOS_TAB_0)*m_num_out*m_data_width;

  // mux
  i_static += (WdecNORp*NOR2_TAB_0+WdecNORn*(NOR2_TAB_1+NOR2_TAB_2+NOR2_TAB_3))/4*(2*m_num_in-1)*m_num_out*m_data_width;

  // control signal inverter
  i_static += (Wdecinvn*NMOS_TAB_0+Wdecinvp*PMOS_TAB_0)*m_num_in*m_num_out;
  return i_static;
}
