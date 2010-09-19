#include "Register.h"

#include <iostream>
#include <cassert>

#include "TechParameter.h"
#include "FlipFlop.h"

using namespace std;

Register::Register(
  uint32_t num_entry_, 
  uint32_t line_width_, 
  const TechParameter *tech_param_ptr_
)
{
  assert(num_entry_ == num_entry_);
  assert(line_width_ == line_width_);

  m_num_entry = num_entry_;
  m_line_width = line_width_;
  m_tech_param_ptr = tech_param_ptr_;

  init();
}

Register::~Register()
{
  delete m_ff_ptr;
}

double Register::calc_e_read() const
{
  // average read energy for one buffer entry
  double e_read = 0;

  
  // for each read operation, the energy consists of one read operation and n write
  // operateion. n means there is n flits in the buffer before read operation.
  // assume n is info->n_entry * 0.25.
  // 
  if (m_num_entry > 1)
  {
    e_read = (m_avg_read + m_num_entry*0.25*m_avg_write);
  }
  else
  {
    e_read = m_avg_read;
  }
  return e_read;
}

double Register::calc_e_write() const
{
  // average write energy for one buffer entry
  double e_write = 0;

  e_write = m_avg_write;
  return e_write;
}

double Register::calc_i_static() const
{
  double i_static = m_ff_ptr->get_i_static()*m_line_width*m_num_entry;

  return i_static;
}

void Register::init()
{
  m_ff_ptr = new FlipFlop("NEG_DFF", 0, m_tech_param_ptr);

  uint32_t num_clock = m_line_width;
  m_avg_read = m_ff_ptr->get_e_clock()*((double)num_clock)/2.0;

  double num_switch = m_line_width/2.0;
  m_avg_write = m_ff_ptr->get_e_switch()*num_switch+m_ff_ptr->get_e_clock()*num_clock;
  return;
}
