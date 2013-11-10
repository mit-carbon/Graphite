#include "link_power_model.h"

LinkPowerModel::LinkPowerModel()
   : _total_dynamic_energy(0.0)
   , _total_static_energy(0.0)
   , _last_energy_compute_time(Time(0))
{}

LinkPowerModel::~LinkPowerModel()
{}
