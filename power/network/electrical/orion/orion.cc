#include "orion.h"

#include <cstring>

#include "SIM_util.h"
#include "SIM_clock.h"
#include "SIM_link.h"

OrionRouter::OrionRouter(
  u_int num_input_port_,
  u_int num_output_port_,
  u_int num_vclass_,
  u_int num_vchannel_,
  u_int num_buf_,
  u_int flit_width_bit_
)
{
  init(num_input_port_, num_output_port_, num_vclass_, num_vchannel_, num_buf_, flit_width_bit_);
}

OrionRouter::~OrionRouter()
{}

void OrionRouter::init(
  u_int num_input_port_,
  u_int num_output_port_,
  u_int num_vclass_,
  u_int num_vchannel_,
  u_int num_buf_,
  u_int flit_width_bit_
)
{
	memset(&m_router_info, 0, sizeof(SIM_router_info_t));
   u_int line_width;
	bool is_outdrv;

	/* PHASE 1: set parameters */
	/* general parameters */
	m_router_info.n_in = num_input_port_;
	m_router_info.n_total_in = m_router_info.n_in;
	m_router_info.n_out = num_output_port_;
	m_router_info.n_total_out = m_router_info.n_out;
	m_router_info.flit_width = flit_width_bit_;

	/* virtual channel parameters */
	m_router_info.n_v_channel = num_vchannel_;
	m_router_info.n_v_class = num_vclass_;

	/* shared buffer implies buffer has tags, so no virtual class -> no sharing */
	/* separate buffer & shared switch implies buffer has tri-state output driver, so no v class -> no sharing */
	if (m_router_info.n_v_class * m_router_info.n_v_channel > 1) {
		m_router_info.in_share_buf = PARM(in_share_buf);
		m_router_info.out_share_buf = PARM(out_share_buf);
		m_router_info.in_share_switch = PARM(in_share_switch);
		m_router_info.out_share_switch = PARM(out_share_switch);
	}
	else {
		m_router_info.in_share_buf = 0;
		m_router_info.out_share_buf = 0;
		m_router_info.in_share_switch = 0;
		m_router_info.out_share_switch = 0;
	}

	/* crossbar */
	m_router_info.crossbar_model = PARM(crossbar_model);
	m_router_info.degree = PARM(crsbar_degree);
	m_router_info.connect_type = PARM(connect_type);
	m_router_info.trans_type = PARM(trans_type);
	m_router_info.xb_in_seg = PARM(xb_in_seg);
	m_router_info.xb_out_seg = PARM(xb_out_seg);
	m_router_info.crossbar_in_len = PARM(crossbar_in_len);
	m_router_info.crossbar_out_len = PARM(crossbar_out_len);
	/* HACK HACK HACK */
	m_router_info.exp_xb_model = PARM(exp_xb_model);
	m_router_info.exp_in_seg = PARM(exp_in_seg);
	m_router_info.exp_out_seg = PARM(exp_out_seg);

	/* input buffer */
	m_router_info.in_buf = 1;
	m_router_info.in_buffer_model = PARM(in_buffer_type);
	is_outdrv = !m_router_info.in_share_buf && m_router_info.in_share_switch;
	SIM_array_init(&m_router_info.in_buf_info, 1, PARM(in_buf_rport), 1, num_buf_, m_router_info.flit_width, is_outdrv, m_router_info.in_buffer_model);

	/* switch allocator input port arbiter */
	if (m_router_info.n_v_class * m_router_info.n_v_channel > 1) {
    m_router_info.sw_in_arb_model = PARM(sw_in_arb_model);
		if (m_router_info.sw_in_arb_model) {
			if (PARM(sw_in_arb_model) == QUEUE_ARBITER) {
				SIM_array_init(&m_router_info.sw_in_arb_queue_info, 1, 1, 1, m_router_info.n_v_class*m_router_info.n_v_channel, SIM_logtwo(m_router_info.n_v_class*m_router_info.n_v_channel), 0, REGISTER);
				m_router_info.sw_in_arb_ff_model = SIM_NO_MODEL;
			}
			else
				m_router_info.sw_in_arb_ff_model = PARM(sw_in_arb_ff_model);
		}
		else
			m_router_info.sw_in_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		m_router_info.sw_in_arb_model = SIM_NO_MODEL;
		m_router_info.sw_in_arb_ff_model = SIM_NO_MODEL;
	}
	/* switch allocator output port arbiter */
	if (m_router_info.n_total_in > 2) {
    m_router_info.sw_out_arb_model = PARM(sw_out_arb_model); 
		if (m_router_info.sw_out_arb_model) {
			if (PARM(sw_out_arb_model) == QUEUE_ARBITER) {
				line_width = SIM_logtwo(m_router_info.n_total_in - 1);
				SIM_array_init(&m_router_info.sw_out_arb_queue_info, 1, 1, 1, m_router_info.n_total_in - 1, line_width, 0, REGISTER);
				m_router_info.sw_out_arb_ff_model = SIM_NO_MODEL;
			}
			else
				m_router_info.sw_out_arb_ff_model = PARM(sw_out_arb_ff_model);
		}
		else
			m_router_info.sw_out_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		m_router_info.sw_out_arb_model = SIM_NO_MODEL;
		m_router_info.sw_out_arb_ff_model = SIM_NO_MODEL;
	}

	/* virtual channel allocator type */
	if (m_router_info.n_v_channel > 1) {
		m_router_info.vc_allocator_type = PARM(vc_allocator_type);
	} 
	else
		m_router_info.vc_allocator_type = SIM_NO_MODEL;

	/* virtual channel allocator input port arbiter */
	if ( m_router_info.n_v_channel > 1 && m_router_info.n_in > 1) {
    m_router_info.vc_in_arb_model = PARM(vc_in_arb_model);
		if (m_router_info.vc_in_arb_model) {
			if (PARM(vc_in_arb_model) == QUEUE_ARBITER) { 
				SIM_array_init(&m_router_info.vc_in_arb_queue_info, 1, 1, 1, m_router_info.n_v_channel, SIM_logtwo(m_router_info.n_v_channel), 0, REGISTER);
				m_router_info.vc_in_arb_ff_model = SIM_NO_MODEL;
			}
			else
				m_router_info.vc_in_arb_ff_model = PARM(vc_in_arb_ff_model);
		}
		else
			m_router_info.vc_in_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		m_router_info.vc_in_arb_model = SIM_NO_MODEL;
		m_router_info.vc_in_arb_ff_model = SIM_NO_MODEL;
	}

	/* virtual channel allocator output port arbiter */
	if (m_router_info.n_in > 1 && m_router_info.n_v_channel > 1) {
    m_router_info.vc_out_arb_model = PARM(vc_out_arb_model);
		if (m_router_info.vc_out_arb_model) {
			if (PARM(vc_out_arb_model) == QUEUE_ARBITER) {
				line_width = SIM_logtwo((m_router_info.n_total_in - 1)*m_router_info.n_v_channel);
				SIM_array_init(&m_router_info.vc_out_arb_queue_info, 1, 1, 1, (m_router_info.n_total_in -1) * m_router_info.n_v_channel, line_width, 0, REGISTER);
				m_router_info.vc_out_arb_ff_model = SIM_NO_MODEL;
			}
			else
				m_router_info.vc_out_arb_ff_model = PARM(vc_out_arb_ff_model);
		}
		else
			m_router_info.vc_out_arb_ff_model = SIM_NO_MODEL;
	}
	else {
		m_router_info.vc_out_arb_model = SIM_NO_MODEL;
		m_router_info.vc_out_arb_ff_model = SIM_NO_MODEL;
	}

	/*virtual channel allocation vc selection model */
	m_router_info.vc_select_buf_type = PARM(vc_select_buf_type);
	if (PARM(vc_allocator_type) == VC_SELECT && m_router_info.n_v_channel > 1 && m_router_info.n_in > 1) {
		m_router_info.vc_select_buf_type = PARM(vc_select_buf_type);
		SIM_array_init(&m_router_info.vc_select_buf_info, 1, 1, 1, m_router_info.n_v_channel, SIM_logtwo(m_router_info.n_v_channel), 0, m_router_info.vc_select_buf_type);
	}
	else {
		m_router_info.vc_select_buf_type = SIM_NO_MODEL;
	}


	/* redundant fields */
	if (m_router_info.in_buf) {
		if (m_router_info.in_share_buf)
			m_router_info.in_n_switch = m_router_info.in_buf_info.read_ports;
		else if (m_router_info.in_share_switch)
			m_router_info.in_n_switch = 1;
		else
			m_router_info.in_n_switch = m_router_info.n_v_class * m_router_info.n_v_channel;
	}
	else
		m_router_info.in_n_switch = 1;

	m_router_info.n_switch_in = m_router_info.n_in * m_router_info.in_n_switch;

	/* no buffering for local output ports */
	m_router_info.n_switch_out = m_router_info.n_out;

	/* clock related parameters */	
	m_router_info.pipeline_stages = PARM(pipeline_stages);
	m_router_info.H_tree_clock = PARM(H_tree_clock);
	m_router_info.router_diagonal = PARM(router_diagonal);

	/* PHASE 2: initialization */
	SIM_router_power_init(&m_router_info, &m_router_power);
	SIM_router_area_init(&m_router_info, &m_router_area);

  /* PHASE 3 */
  m_dynamic_energy_buf_read = SIM_array_stat_energy(&m_router_info.in_buf_info, &m_router_power.in_buf, 1, 0, 0, NULL, 0);
  m_dynamic_energy_buf_write = SIM_array_stat_energy(&m_router_info.in_buf_info, &m_router_power.in_buf, 0, 1, 0, NULL, 0);
  m_static_power_buf = m_router_power.I_buf_static * Vdd * SCALE_S;

  m_dynamic_energy_xbar = SIM_crossbar_stat_energy(&m_router_power.crossbar, 0, NULL, 0, 1) / (double)m_router_info.flit_width;
  m_static_power_xbar = m_router_power.I_crossbar_static * Vdd * SCALE_S;

  m_static_power_sa = m_router_power.I_sw_arbiter_static * Vdd * SCALE_S;

  m_static_power_va = m_router_power.I_vc_arbiter_static * Vdd * SCALE_S;
  if(m_router_info.vc_allocator_type == VC_SELECT && (m_router_info.n_v_channel > 1) && (m_router_info.n_in > 1))
  {
    m_dynamic_energy_vc_select_read = SIM_array_stat_energy(&m_router_info.vc_select_buf_info, &m_router_power.vc_select_buf, 1, 0, 0, NULL, 0); 
    m_dynamic_energy_vc_select_write = SIM_array_stat_energy(&m_router_info.vc_select_buf_info, &m_router_power.vc_select_buf, 0, 1, 0, NULL, 0); 
  }
  else
  {
    m_dynamic_energy_vc_select_read = 0.0;
    m_dynamic_energy_vc_select_write = 0.0;
  }

  m_dynamic_energy_clock = SIM_total_clockEnergy(&m_router_info, &m_router_power);
  m_static_power_clock = m_router_power.I_clock_static * Vdd * SCALE_S;

	return;
}

double OrionRouter::calc_dynamic_energy_buf(bool is_read_)
{
  if(is_read_)
    return m_dynamic_energy_buf_read;
  else
    return m_dynamic_energy_buf_write;
}

double OrionRouter::calc_dynamic_energy_xbar(u_int num_bit_flips_)
{
  return m_dynamic_energy_xbar * num_bit_flips_;
}

double OrionRouter::calc_dynamic_energy_local_vc_arb(u_int num_req_)
{
  if((m_router_info.vc_allocator_type == TWO_STAGE_ARB) && (m_router_info.vc_in_arb_model))
    //return SIM_arbiter_stat_energy(&m_router_power.vc_in_arb, &m_router_info.vc_in_arb_queue_info, m_router_info.n_v_channel/2.0, 0, NULL, 0);
    return SIM_arbiter_stat_energy(&m_router_power.vc_in_arb, &m_router_info.vc_in_arb_queue_info, num_req_, 0, NULL, 0);
  else
    return 0.0;
}

double OrionRouter::calc_dynamic_energy_global_vc_arb(u_int num_req_)
{
  if((m_router_info.vc_allocator_type == ONE_STAGE_ARB) && (m_router_info.vc_out_arb_model))
    //return SIM_arbiter_stat_energy(&m_router_power.vc_out_arb, &m_router_info.vc_out_arb_queue_info, 1, 0, NULL, 0);
    return SIM_arbiter_stat_energy(&m_router_power.vc_out_arb, &m_router_info.vc_out_arb_queue_info, num_req_, 0, NULL, 0);
  else if((m_router_info.vc_allocator_type == TWO_STAGE_ARB) && (m_router_info.vc_out_arb_model))
    //return SIM_arbiter_stat_energy(&m_router_power.vc_out_arb, &m_router_info.vc_out_arb_queue_info, 2, 0, NULL, 0);
    return SIM_arbiter_stat_energy(&m_router_power.vc_out_arb, &m_router_info.vc_out_arb_queue_info, num_req_, 0, NULL, 0);
  else
    return 0.0;
}

double OrionRouter::calc_dynamic_energy_vc_select(bool is_read_)
{
  if(is_read_)
    return m_dynamic_energy_vc_select_read;
  else
    return m_dynamic_energy_vc_select_write;
}

double OrionRouter::calc_dynamic_energy_local_sw_arb(u_int num_req_)
{
  if(m_router_info.sw_in_arb_model)
		// assume (info->n_v_channel*info->n_v_class)/2 vcs are making request at each arbiter
    //return SIM_arbiter_stat_energy(&m_router_power.sw_in_arb, &m_router_info.sw_in_arb_queue_info, (m_router_info.n_v_channel*m_router_info.n_v_class)/2.0, 0, NULL, 0);
    return SIM_arbiter_stat_energy(&m_router_power.sw_in_arb, &m_router_info.sw_in_arb_queue_info, num_req_, 0, NULL, 0);
  else
    return 0.0;
}

double OrionRouter::calc_dynamic_energy_global_sw_arb(u_int num_req_)
{
  if(m_router_info.sw_out_arb_model)
		// assume (info->n_in)/2 vcs are making request at each arbiter
    //return SIM_arbiter_stat_energy(&m_router_power.sw_out_arb, &m_router_info.sw_out_arb_queue_info, (m_router_info.n_in)/2.0, 0, NULL, 0);
    return SIM_arbiter_stat_energy(&m_router_power.sw_out_arb, &m_router_info.sw_out_arb_queue_info, num_req_, 0, NULL, 0);
  else
    return 0.0;
}

OrionLink::OrionLink(
  double link_length_,
  u_int flit_width_bit_
)
{
  init(link_length_, flit_width_bit_);
}

OrionLink::~OrionLink()
{}

void OrionLink::init(
  double link_length_,
  u_int flit_width_bit_
)
{
  m_link_length = link_length_;
  m_flit_width_bit = flit_width_bit_;

  m_dynamic_energy_per_bit = LinkDynamicEnergyPerBitPerMeter(m_link_length, Vdd) * m_link_length;
  m_static_power = LinkLeakagePowerPerMeter(m_link_length, Vdd) * m_link_length * m_flit_width_bit;
}

double OrionLink::calc_dynamic_energy(u_int num_bit_flip_)
{
  return m_dynamic_energy_per_bit * num_bit_flip_;
}
