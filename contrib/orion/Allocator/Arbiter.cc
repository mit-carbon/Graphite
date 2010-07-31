#include "Arbiter.h"

#include <iostream>

#include "TechParameter.h"
#include "RRArbiter.h"
#include "MatrixArbiter.h"

using namespace std;

Arbiter::Arbiter(
  const ArbiterModel arb_model_,
  const uint32_t req_width_,
  const double len_in_wire_,
  const TechParameter* tech_param_ptr_
)
{
  m_arb_model = arb_model_;
  m_req_width = req_width_;
  m_len_in_wire = len_in_wire_;
  m_tech_param_ptr = tech_param_ptr_;
}

Arbiter::~Arbiter()
{}

double Arbiter::get_static_power() const
{
  double vdd = m_tech_param_ptr->get_vdd();
  double SCALE_S = m_tech_param_ptr->get_SCALE_S();

  return m_i_static*vdd*SCALE_S;
}

Arbiter* Arbiter::create_arbiter(
  const string& arb_model_str_, 
  const string& ff_model_str_,
  uint32_t req_width_, 
  double len_in_wire_, 
  const TechParameter* tech_param_ptr_
)
{
  if (arb_model_str_ == string("RR_ARBITER"))
  {
    return new RRArbiter(ff_model_str_, req_width_, len_in_wire_, tech_param_ptr_);
  }
  else if (arb_model_str_ == string("MATRIX_ARBITER"))
  {
    return new MatrixArbiter(ff_model_str_, req_width_, len_in_wire_, tech_param_ptr_);
  }
  else
  {
    cerr << "WARNING: No Arbiter model" << endl;
    return (Arbiter*)NULL;
  }
}
