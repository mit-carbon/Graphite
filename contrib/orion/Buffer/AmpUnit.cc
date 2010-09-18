#include "AmpUnit.h"

#include "TechParameter.h"

AmpUnit::AmpUnit(
  const string& amp_model_str_,
  const TechParameter* tech_param_ptr_
)
{
  if (amp_model_str_.compare("GENERIC_AMP") == 0)
  {
    m_amp_model = GENERIC_AMP;
  }
  else
  {
    m_amp_model = NO_MODEL;
  }

  if (m_amp_model != NO_MODEL)
  {
    m_tech_param_ptr = tech_param_ptr_;

    init();
  }
}

AmpUnit::~AmpUnit()
{}

void AmpUnit::init()
{
  double vdd = m_tech_param_ptr->get_vdd();
  double period = m_tech_param_ptr->get_period();
  double amp_Idsat = m_tech_param_ptr->get_amp_idsat();

  m_e_access = (vdd / 8.0 * period * amp_Idsat);
  return;
}


