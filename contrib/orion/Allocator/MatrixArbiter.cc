#include "MatrixArbiter.h"

#include <iostream>
#include <cmath>

#include "TechParameter.h"
#include "FlipFlop.h"

using namespace std;

MatrixArbiter::MatrixArbiter(
  const string& ff_model_str_,
  uint32_t req_width_,
  double len_in_wire_,
  const TechParameter* tech_param_ptr_
) : Arbiter(RR_ARBITER, req_width_, len_in_wire_, tech_param_ptr_)
{
  init(ff_model_str_);
}

MatrixArbiter::~MatrixArbiter()
{
  delete m_ff_ptr;
}

double MatrixArbiter::calc_dynamic_energy(double num_req_, bool is_max_) const
{
  if (num_req_ > m_req_width)
  {
    cerr << "WARNING: (num_req_ > m_req_width). Set num_req_ = m_req_width" << endl;
    num_req_ = m_req_width;
  }

  double num_grant;
  if (num_req_ >= 1) num_grant = 1;
  else if (num_req_) num_grant = 1.0 / ceil(1.0/num_req_);
  else num_grant = 0;

  uint32_t total_pri = m_req_width*(m_req_width-1)/2;
  double num_chg_pri = (m_req_width-1)*(is_max_? 1:0.5);

  double e_atomic;
  double e_arb = 0;

  //FIXME: we may overestimate request switch
  e_atomic = m_e_chg_req*num_req_;
  e_arb += e_atomic;

  e_atomic = m_e_chg_grant*num_grant;
  e_arb += e_atomic;

  // priority register
  e_atomic = m_ff_ptr->get_e_switch()*num_chg_pri*num_grant;
  e_arb += e_atomic;

  // assume 1 and 0 are uniformly distributed
  if ((m_ff_ptr->get_e_keep_0() >= m_ff_ptr->get_e_keep_1()) || (!is_max_))
  {
    e_atomic = m_ff_ptr->get_e_keep_0()*(total_pri-num_chg_pri*num_grant)*(is_max_? 1:0.5);
    e_arb += e_atomic;
  }
  if ((m_ff_ptr->get_e_keep_0() < m_ff_ptr->get_e_keep_1()) || (!is_max_))
  {
    e_atomic = m_ff_ptr->get_e_keep_1()*(total_pri-num_chg_pri*num_grant)*(is_max_? 1:0.5);
    e_arb += e_atomic;
  }

  e_atomic = m_ff_ptr->get_e_clock()*total_pri;
  e_arb += e_atomic;

  // based on above assumptions
  if (is_max_)
  {
    //min(p,n/2)(n-1)+2(n-1)
    e_atomic = m_e_chg_int*(min(num_req_, m_req_width*0.5)+2)*(m_req_width-1);
  }
  else
  {
    //p(n-1)/2+(n-1)/2
    e_atomic = m_e_chg_int*(num_req_+1)*(m_req_width-1)*0.5;
  }
  e_arb += e_atomic;

  return e_arb;
}

void MatrixArbiter::init(const string& ff_model_str_)
{
  double e_factor = m_tech_param_ptr->get_EnergyFactor();

  m_e_chg_req = calc_req_cap()/2*e_factor;
  // two grant signals switch together, so no 1/2
  m_e_chg_grant = calc_grant_cap()*e_factor;
  m_e_chg_int = calc_int_cap()/2*e_factor;

  double ff_load = calc_pri_cap();
  m_ff_ptr = new FlipFlop(ff_model_str_, ff_load, m_tech_param_ptr);

  m_i_static = calc_i_static();
  return;
}

// the "huge" NOR gate in matrix arbiter model is an approximation
// switch cap of request signal
double MatrixArbiter::calc_req_cap()
{
  double total_cap = 0;

  // part 1: gate cap of NOR gates
  // FIXME: need actual size
  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  total_cap += (m_req_width-1)*m_tech_param_ptr->calc_gatecap(WdecNORn+WdecNORp, 0);

  // part 2: inverter
  // FIXME: need actual size
  double Wdecinvn = m_tech_param_ptr->get_Wdecinvn();
  double Wdecinvp = m_tech_param_ptr->get_Wdecinvp();
  total_cap += m_tech_param_ptr->calc_draincap(Wdecinvn, TechParameter::NCH, 1)
    + m_tech_param_ptr->calc_draincap(Wdecinvp, TechParameter::PCH, 1)
    + m_tech_param_ptr->calc_gatecap(Wdecinvn+Wdecinvp, 0);

  // part 3: gate cap of the "huge" NOR gate
  // FIXME: need actual size
  total_cap += m_tech_param_ptr->calc_gatecap(WdecNORn+WdecNORp, 0);

  // part 4: wire cap
  double Cmetal = m_tech_param_ptr->get_Cmetal();
  total_cap += m_len_in_wire*Cmetal;

  return total_cap;
}

// switch cap of priority signal
double MatrixArbiter::calc_pri_cap()
{
  double total_cap = 0;

  // part 1: gate cap of NOR gate
  // FIXME: need actual size
  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  total_cap += 2*m_tech_param_ptr->calc_gatecap(WdecNORn+WdecNORp, 0);

  return total_cap;
}

// switch cap of grant signa
double MatrixArbiter::calc_grant_cap()
{
  double total_cap = 0;

  // part 1: drain cap of NOR gate
  // FIXME: need actual size
  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  total_cap += m_req_width*m_tech_param_ptr->calc_draincap(WdecNORn, TechParameter::NCH, 1)
    + m_tech_param_ptr->calc_draincap(WdecNORp, TechParameter::PCH, m_req_width);

  return total_cap;
}

// switch cap of internal node
double MatrixArbiter::calc_int_cap()
{
  double total_cap = 0;

  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  // part 1: drain cap of NOR gate (this bloc)
  // FIXME: need actual size
  total_cap += 2*m_tech_param_ptr->calc_draincap(WdecNORn, TechParameter::NCH, 1)
    + m_tech_param_ptr->calc_draincap(WdecNORp, TechParameter::PCH, 2);

  // part 2: gate cap of NOR gate (next block)
  // FIXME: need actual size
  total_cap += m_tech_param_ptr->calc_gatecap(WdecNORn+WdecNORp, 0);

  return total_cap;
}

double MatrixArbiter::calc_i_static()
{
  double i_static = 0;

  double WdecNORn = m_tech_param_ptr->get_WdecNORn();
  double WdecNORp = m_tech_param_ptr->get_WdecNORp();
  double Wdecinvn = m_tech_param_ptr->get_Wdecinvn();
  double Wdecinvp = m_tech_param_ptr->get_Wdecinvp();
  double Wdff = m_tech_param_ptr->get_Wdff();
  double NOR2_TAB_0 = m_tech_param_ptr->get_NOR2_TAB(0);
  double NOR2_TAB_1 = m_tech_param_ptr->get_NOR2_TAB(1);
  double NOR2_TAB_2 = m_tech_param_ptr->get_NOR2_TAB(2);
  double NOR2_TAB_3 = m_tech_param_ptr->get_NOR2_TAB(3);
  double NMOS_TAB_0 = m_tech_param_ptr->get_NMOS_TAB(0);
  double PMOS_TAB_0 = m_tech_param_ptr->get_PMOS_TAB(0);
  double DFF_TAB_0 = m_tech_param_ptr->get_DFF_TAB(0);

  // NOR
  i_static += ((2*m_req_width-1)*m_req_width*((WdecNORp*NOR2_TAB_0+WdecNORn*(NOR2_TAB_1+NOR2_TAB_2+NOR2_TAB_3))/4));
  // inverter
  i_static += m_req_width*((Wdecinvn*NMOS_TAB_0+Wdecinvp*PMOS_TAB_0)/2);
  // dff
  i_static += (m_req_width*(m_req_width-1)/2)*Wdff*DFF_TAB_0;

  return i_static;
}
