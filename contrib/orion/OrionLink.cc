#include "OrionLink.h"

#include <iostream>
#include <cassert>

#include "OrionConfig.h"
#include "Wire.h"

using namespace std;

OrionLink::OrionLink(
  double len_,
  uint32_t line_width_,
  const OrionConfig* orion_cfg_ptr_
)
{
  assert(len_ == len_);
  assert(line_width_ == line_width_);

  m_len = len_;
  m_line_width = line_width_;
  m_orion_cfg_ptr = orion_cfg_ptr_;

  init();
}

OrionLink::~OrionLink()
{}

double OrionLink::calc_dynamic_energy(uint32_t num_bit_flip_) const
{
  assert(num_bit_flip_ <= m_line_width);

  return (num_bit_flip_*(m_dynamic_energy_per_bit/2));
}

double OrionLink::get_static_power() const
{
  return (m_line_width*m_static_power_per_bit);
}

void OrionLink::init()
{
  const TechParameter* tech_param_ptr = m_orion_cfg_ptr->get_tech_param_ptr();

  const string& width_spacing_model_str = m_orion_cfg_ptr->get<string>("WIRE_WIDTH_SPACING");
  const string& buf_scheme_str = m_orion_cfg_ptr->get<string>("WIRE_BUFFERING_MODEL");
  bool is_shielding = m_orion_cfg_ptr->get<bool>("WIRE_IS_SHIELDING");
  Wire wire(width_spacing_model_str, buf_scheme_str, is_shielding, tech_param_ptr);

  m_dynamic_energy_per_bit = wire.calc_dynamic_energy(m_len);
  m_static_power_per_bit = wire.calc_static_power(m_len);
  return;
}

void OrionLink::print() const
{
  cout << "Link - Dynamic Energy" << endl;
  cout << "\t" << "One Bit = " << calc_dynamic_energy(1) << endl;
  cout << endl;
  cout << "Link - Static Power" << endl;
  cout << "\t" << "One Bit = " << get_static_power() << endl;
  cout << endl;
  return;
}
