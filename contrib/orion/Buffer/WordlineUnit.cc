#include "WordlineUnit.h"

#include "SRAM.h"
#include "TechParameter.h"

WordlineUnit::WordlineUnit(
  const string& wl_model_str_,
  const SRAM* sram_ptr_,
  const TechParameter* tech_param_ptr_
)
{
  if (wl_model_str_ == string("RW_WORDLINE"))
  {
    m_wl_model = RW_WORDLINE;
  }
  else if (wl_model_str_ == string("WO_WORDLINE"))
  {
    m_wl_model = WO_WORDLINE;
  }
  else 
  {
    m_wl_model = NO_MODEL;
  }

  if (m_wl_model != NO_MODEL)
  {
    m_sram_ptr = sram_ptr_;
    m_tech_param_ptr = tech_param_ptr_;

    init();
  }
}

WordlineUnit::~WordlineUnit()
{}

void WordlineUnit::init()
{
  uint32_t num_port = m_sram_ptr->get_num_port();
  uint32_t num_read_port = m_sram_ptr->get_num_read_port();
  uint32_t num_col = m_sram_ptr->get_num_col();
  uint32_t num_data_end = m_sram_ptr->get_num_data_end();
  double RegCellWidth = m_tech_param_ptr->get_RegCellWidth();
  double BitlineSpacing = m_tech_param_ptr->get_BitlineSpacing();

  if (num_data_end == 2)
  {
    m_wl_len = num_col*(RegCellWidth + 2*num_port*BitlineSpacing);
  }
  else
  {
    m_wl_len = num_col*(RegCellWidth + (2*num_port-num_read_port)*BitlineSpacing);
  }

  double wl_cmetal;
  if (num_port > 1)
  {
    wl_cmetal = m_tech_param_ptr->get_CC3M3metal();
  }
  else
  {
    wl_cmetal = m_tech_param_ptr->get_CM3metal();
  }

  m_wl_wire_cap = m_wl_len*wl_cmetal;

  double e_factor = m_tech_param_ptr->get_EnergyFactor();
  double Wmemcellr = m_tech_param_ptr->get_Wmemcellr();
  double Wmemcellw = m_tech_param_ptr->get_Wmemcellw();
  double Woutdrivern = m_tech_param_ptr->get_Woutdrivern();
  double Woutdriverp = m_tech_param_ptr->get_Woutdriverp();
  double NMOS_TAB_0 = m_tech_param_ptr->get_NMOS_TAB(0);
  double PMOS_TAB_0 = m_tech_param_ptr->get_PMOS_TAB(0);
  switch(m_wl_model)
  {
    case RW_WORDLINE:
      m_e_read = calc_wordline_cap(num_col*num_data_end, Wmemcellr) * e_factor;
      m_e_write = calc_wordline_cap(num_col*2, Wmemcellw) * e_factor;
      m_i_static = (Woutdrivern*NMOS_TAB_0 + Woutdriverp*PMOS_TAB_0);
      break;
    case WO_WORDLINE:
      m_e_read = 0;
      m_e_write = calc_wordline_cap(num_col*2, Wmemcellw)*e_factor;
      m_i_static = 0;
      break;
    default:
      printf("error\n");
  }
  return;
}

double WordlineUnit::calc_wordline_cap(
  uint32_t num_mos_,
  double mos_width_
) const
{
  double total_cap;

  // part 1: line cap, including gate cap of pass tx's and metal cap
  double BitWidth = m_tech_param_ptr->get_BitWidth();
  total_cap = m_tech_param_ptr->calc_gatecappass(mos_width_, BitWidth/2.0-mos_width_)*num_mos_ + m_wl_wire_cap;

  // part 2: input driver
  double period = m_tech_param_ptr->get_period();
  double psize, nsize;
  psize = m_tech_param_ptr->calc_driver_psize(total_cap, period/16.0);
  double Wdecinvn = m_tech_param_ptr->get_Wdecinvn();
  double Wdecinvp = m_tech_param_ptr->get_Wdecinvp();
  nsize = psize*Wdecinvn/Wdecinvp;

  // WHS: 20 should go to PARM
  total_cap += m_tech_param_ptr->calc_draincap(nsize, TechParameter::NCH, 1) + m_tech_param_ptr->calc_draincap(psize, TechParameter::PCH, 1) + m_tech_param_ptr->calc_gatecap(psize+nsize, 20);

  return total_cap;
}

