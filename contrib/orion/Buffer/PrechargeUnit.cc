#include "PrechargeUnit.h"

#include "TechParameter.h"
#include "SRAM.h"

PrechargeUnit::PrechargeUnit(
  const string& pre_model_str_,
  double pre_load_,
  const SRAM* sram_ptr_,
  const TechParameter* tech_param_ptr_
)
{
  if (pre_model_str_ == "SINGLE_BITLINE")
  {
    m_pre_model = SINGLE_BITLINE;
  }
  else if (pre_model_str_ == "EQU_BITLINE")
  {
    m_pre_model = EQU_BITLINE;
  }
  else if (pre_model_str_ == "SINGLE_OTHER")
  {
    m_pre_model = SINGLE_OTHER;
  }
  else
  {
    m_pre_model = NO_MODEL;
  }

  if (m_pre_model != NO_MODEL)
  {
    m_pre_load = pre_load_;
    m_sram_ptr = sram_ptr_;
    m_tech_param_ptr = tech_param_ptr_;

    init();
  }
}

PrechargeUnit::~PrechargeUnit()
{
}

void PrechargeUnit::init()
{
  double period = m_tech_param_ptr->get_period();
  m_pre_size = m_tech_param_ptr->calc_driver_psize(m_pre_load, period/8.0);
  //FIXME - shouldn't be added
  double Wdecinvn = m_tech_param_ptr->get_Wdecinvn();
  double Wdecinvp = m_tech_param_ptr->get_Wdecinvp();
  m_pre_size += m_pre_size*Wdecinvn/Wdecinvp;

  uint32_t num_gate = calc_num_pre_gate();

  // WHS: 10 should go to PARM 
  double e_factor = m_tech_param_ptr->get_EnergyFactor();
  m_e_charge_gate = calc_pre_cap(m_pre_size, 10) * num_gate * e_factor;

  uint32_t num_data_end = m_sram_ptr->get_num_data_end();
  if (num_data_end == 2)
  {
    e_factor = m_tech_param_ptr->get_SenseEnergyFactor();
  }
  else
  {
    e_factor = m_tech_param_ptr->get_EnergyFactor();
  }
  uint32_t num_drain = calc_num_pre_drain();
  m_e_charge_drain = m_tech_param_ptr->calc_draincap(m_pre_size, TechParameter::PCH, 1)*num_drain*e_factor;

  // static power
  double PMOS_TAB_0 = m_tech_param_ptr->get_PMOS_TAB(0);
  m_i_static = num_gate*m_pre_size*PMOS_TAB_0;
}

uint32_t PrechargeUnit::calc_num_pre_gate()
{
  switch(m_pre_model)
  {
    case SINGLE_BITLINE: return 2;
    case EQU_BITLINE:    return 3;
    case SINGLE_OTHER:   return 1;
    default: printf("error\n"); return 0;
  }
}

uint32_t PrechargeUnit::calc_num_pre_drain()
{
  switch(m_pre_model)
  {
    case SINGLE_BITLINE: return 1;
    case EQU_BITLINE:    return 2;
    case SINGLE_OTHER:   return 1;
    default: printf("error\n"); return 0;
  }
}

double PrechargeUnit::calc_pre_cap(double width_, double length_)
{
  return m_tech_param_ptr->calc_gatecap(width_, length_);
}

