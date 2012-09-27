/*------------------------------------------------------------
 *                              CACTI 6.5
 *         Copyright 2008 Hewlett-Packard Development Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein, and
 * hereby grant back to Hewlett-Packard Company and its affiliated companies ("HP")
 * a non-exclusive, unrestricted, royalty-free right and license under any changes,
 * enhancements or extensions  made to the core functions of the software, including
 * but not limited to those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to HP any such changes,
 * enhancements or extensions that they make and inform HP of noteworthy uses of
 * this software.  Correspondence should be provided to HP at:
 *
 *                       Director of Intellectual Property Licensing
 *                       Office of Strategy and Technology
 *                       Hewlett-Packard Company
 *                       1501 Page Mill Road
 *                       Palo Alto, California  94304
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND HP DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL HP
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/




#include "basic_circuit.h"
#include "parameter.h"
#include <iostream>
#include <assert.h>
#include <cmath>

namespace McPAT
{

uint32_t _log2(uint64_t num)
{
  uint32_t log2 = 0;

  if (num == 0)
  {
    std::cerr << "log0?" << std::endl;
    exit(1);
  }

  while (num > 1)
  {
    num = (num >> 1);
    log2++;
  }

  return log2;
}


bool is_pow2(int64_t val)
{
  if (val <= 0)
  {
    return false;
  }
  else if (val == 1)
  {
    return true;
  }
  else
  {
    return (_log2(val) != _log2(val-1));
  }
}


int powers (int base, int n)
{
  int i, p;

  p = 1;
  for (i = 1; i <= n; ++i)
    p *= base;
  return p;
}

/*----------------------------------------------------------------------*/

double logtwo (double x)
{
  assert(x > 0);
  return ((double) (log (x) / log (2.0)));
}

/*----------------------------------------------------------------------*/


double gate_C(
    double width,
    double wirelength,
    bool   _is_dram,
    bool   _is_cell,
    bool   _is_wl_tr)
{
  const TechnologyParameter::DeviceType * dt;

  if (_is_dram && _is_cell)
  {
    dt = &g_tp.dram_acc;   //DRAM cell access transistor
  }
  else if (_is_dram && _is_wl_tr)
  {
    dt = &g_tp.dram_wl;    //DRAM wordline transistor
  }
  else if (!_is_dram && _is_cell)
  {
    dt = &g_tp.sram_cell;  // SRAM cell access transistor
  }
  else
  {
    dt = &g_tp.peri_global;
  }

  return (dt->C_g_ideal + dt->C_overlap + 3*dt->C_fringe)*width + dt->l_phy*Cpolywire;
}


// returns gate capacitance in Farads
// actually this function is the same as gate_C() now
double gate_C_pass(
    double width,       // gate width in um (length is Lphy_periph_global)
    double wirelength,  // poly wire length going to gate in lambda
    bool   _is_dram,
    bool   _is_cell,
    bool   _is_wl_tr)
{
  // v5.0
  const TechnologyParameter::DeviceType * dt;

  if ((_is_dram) && (_is_cell))
  {
    dt = &g_tp.dram_acc;   //DRAM cell access transistor
  }
  else if ((_is_dram) && (_is_wl_tr))
  {
    dt = &g_tp.dram_wl;    //DRAM wordline transistor
  }
  else if ((!_is_dram) && _is_cell)
  {
    dt = &g_tp.sram_cell;  // SRAM cell access transistor
  }
  else
  {
    dt = &g_tp.peri_global;
  }

  return (dt->C_g_ideal + dt->C_overlap + 3*dt->C_fringe)*width + dt->l_phy*Cpolywire;
}



double drain_C_(
    double width,
    int nchannel,
    int stack,
    int next_arg_thresh_folding_width_or_height_cell,
    double fold_dimension,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  double w_folded_tr;
  const  TechnologyParameter::DeviceType * dt;

  if ((_is_dram) && (_is_cell))
  {
    dt = &g_tp.dram_acc;   // DRAM cell access transistor
  }
  else if ((_is_dram) && (_is_wl_tr))
  {
    dt = &g_tp.dram_wl;    // DRAM wordline transistor
  }
  else if ((!_is_dram) && _is_cell)
  {
    dt = &g_tp.sram_cell;  // SRAM cell access transistor
  }
  else
  {
    dt = &g_tp.peri_global;
  }

  double c_junc_area = dt->C_junc;
  double c_junc_sidewall = dt->C_junc_sidewall;
  double c_fringe    = 2*dt->C_fringe;
  double c_overlap   = 2*dt->C_overlap;
  double drain_C_metal_connecting_folded_tr = 0;

  // determine the width of the transistor after folding (if it is getting folded)
  if (next_arg_thresh_folding_width_or_height_cell == 0)
  { // interpret fold_dimension as the the folding width threshold
    // i.e. the value of transistor width above which the transistor gets folded
    w_folded_tr = fold_dimension;
  }
  else
  { // interpret fold_dimension as the height of the cell that this transistor is part of.
    double h_tr_region  = fold_dimension - 2 * g_tp.HPOWERRAIL;
    // TODO : w_folded_tr must come from Component::compute_gate_area()
    double ratio_p_to_n = 2.0 / (2.0 + 1.0);
    if (nchannel)
    {
      w_folded_tr = (1 - ratio_p_to_n) * (h_tr_region - g_tp.MIN_GAP_BET_P_AND_N_DIFFS);
    }
    else
    {
      w_folded_tr = ratio_p_to_n * (h_tr_region - g_tp.MIN_GAP_BET_P_AND_N_DIFFS);
    }
  }
  int num_folded_tr = (int) (ceil(width / w_folded_tr));

  if (num_folded_tr < 2)
  {
    w_folded_tr = width;
  }

  double total_drain_w = (g_tp.w_poly_contact + 2 * g_tp.spacing_poly_to_contact) +  // only for drain
                         (stack - 1) * g_tp.spacing_poly_to_poly;
  double drain_h_for_sidewall = w_folded_tr;
  double total_drain_height_for_cap_wrt_gate = w_folded_tr + 2 * w_folded_tr * (stack - 1);
  if (num_folded_tr > 1)
  {
    total_drain_w += (num_folded_tr - 2) * (g_tp.w_poly_contact + 2 * g_tp.spacing_poly_to_contact) +
                     (num_folded_tr - 1) * ((stack - 1) * g_tp.spacing_poly_to_poly);

    if (num_folded_tr%2 == 0)
    {
      drain_h_for_sidewall = 0;
    }
    total_drain_height_for_cap_wrt_gate *= num_folded_tr;
    drain_C_metal_connecting_folded_tr   = g_tp.wire_local.C_per_um * total_drain_w;
  }

  double drain_C_area     = c_junc_area * total_drain_w * w_folded_tr;
  double drain_C_sidewall = c_junc_sidewall * (drain_h_for_sidewall + 2 * total_drain_w);
  double drain_C_wrt_gate = (c_fringe + c_overlap) * total_drain_height_for_cap_wrt_gate;

  return (drain_C_area + drain_C_sidewall + drain_C_wrt_gate + drain_C_metal_connecting_folded_tr);
}


double tr_R_on(
    double width,
    int nchannel,
    int stack,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  const TechnologyParameter::DeviceType * dt;

  if ((_is_dram) && (_is_cell))
  {
    dt = &g_tp.dram_acc;   //DRAM cell access transistor
  }
  else if ((_is_dram) && (_is_wl_tr))
  {
    dt = &g_tp.dram_wl;    //DRAM wordline transistor
  }
  else if ((!_is_dram) && _is_cell)
  {
    dt = &g_tp.sram_cell;  // SRAM cell access transistor
  }
  else
  {
    dt = &g_tp.peri_global;
  }

  double restrans = (nchannel) ? dt->R_nch_on : dt->R_pch_on;
  return (stack * restrans / width);
}


/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */

// returns width in um
double R_to_w(
    double res,
    int   nchannel,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  const TechnologyParameter::DeviceType * dt;

  if ((_is_dram) && (_is_cell))
  {
    dt = &g_tp.dram_acc;   //DRAM cell access transistor
  }
  else if ((_is_dram) && (_is_wl_tr))
  {
    dt = &g_tp.dram_wl;    //DRAM wordline transistor
  }
  else if ((!_is_dram) && (_is_cell))
  {
    dt = &g_tp.sram_cell;  // SRAM cell access transistor
  }
  else
  {
    dt = &g_tp.peri_global;
  }

  double restrans = (nchannel) ? dt->R_nch_on : dt->R_pch_on;
  return (restrans / res);
}


double pmos_to_nmos_sz_ratio(
    bool _is_dram,
    bool _is_wl_tr)
{
  double p_to_n_sizing_ratio;
  if ((_is_dram) && (_is_wl_tr))
  { //DRAM wordline transistor
    p_to_n_sizing_ratio = g_tp.dram_wl.n_to_p_eff_curr_drv_ratio;
  }
  else
  { //DRAM or SRAM all other transistors
    p_to_n_sizing_ratio = g_tp.peri_global.n_to_p_eff_curr_drv_ratio;
  }
  return p_to_n_sizing_ratio;
}


// "Timing Models for MOS Circuits" by Mark Horowitz, 1984
double horowitz(
    double inputramptime, // input rise time
    double tf,            // time constant of gate
    double vs1,           // threshold voltage
    double vs2,           // threshold voltage
    int    rise)          // whether input rises or fall
{
  if (inputramptime == 0 && vs1 == vs2)
  {
    return tf * (vs1 < 1 ? -log(vs1) : log(vs1));
  }
  double a, b, td;

  a = inputramptime / tf;
  if (rise == RISE)
  {
    b = 0.5;
    td = tf * sqrt(log(vs1)*log(vs1) + 2*a*b*(1.0 - vs1)) + tf*(log(vs1) - log(vs2));
  }
  else
  {
    b = 0.4;
    td = tf * sqrt(log(1.0 - vs1)*log(1.0 - vs1) + 2*a*b*(vs1)) + tf*(log(1.0 - vs1) - log(1.0 - vs2));
  }
  return (td);
}

double cmos_Ileak(
    double nWidth,
    double pWidth,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  TechnologyParameter::DeviceType * dt;

  if ((!_is_dram)&&(_is_cell))
  { //SRAM cell access transistor
    dt = &(g_tp.sram_cell);
  }
  else if ((_is_dram)&&(_is_wl_tr))
  { //DRAM wordline transistor
    dt = &(g_tp.dram_wl);
  }
  else
  { //DRAM or SRAM all other transistors
    dt = &(g_tp.peri_global);
  }
  return nWidth*dt->I_off_n + pWidth*dt->I_off_p;
}


double simplified_nmos_leakage(
    double nwidth,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  TechnologyParameter::DeviceType * dt;

  if ((!_is_dram)&&(_is_cell))
  { //SRAM cell access transistor
    dt = &(g_tp.sram_cell);
  }
  else if ((_is_dram)&&(_is_wl_tr))
  { //DRAM wordline transistor
    dt = &(g_tp.dram_wl);
  }
  else
  { //DRAM or SRAM all other transistors
    dt = &(g_tp.peri_global);
  }
  return nwidth * dt->I_off_n;
}

int factorial(int n, int m)
{
	int fa = m, i;
	for (i=m+1; i<=n; i++)
		fa *=i;
	return fa;
}

int combination(int n, int m)
{
  int ret;
  ret = factorial(n, m+1) / factorial(n - m);
  return ret;
}

double simplified_pmos_leakage(
    double pwidth,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  TechnologyParameter::DeviceType * dt;

  if ((!_is_dram)&&(_is_cell))
  { //SRAM cell access transistor
    dt = &(g_tp.sram_cell);
  }
  else if ((_is_dram)&&(_is_wl_tr))
  { //DRAM wordline transistor
    dt = &(g_tp.dram_wl);
  }
  else
  { //DRAM or SRAM all other transistors
    dt = &(g_tp.peri_global);
  }
  return pwidth * dt->I_off_p;
}

double cmos_Ig_n(
    double nWidth,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  TechnologyParameter::DeviceType * dt;

  if ((!_is_dram)&&(_is_cell))
  { //SRAM cell access transistor
    dt = &(g_tp.sram_cell);
  }
  else if ((_is_dram)&&(_is_wl_tr))
  { //DRAM wordline transistor
    dt = &(g_tp.dram_wl);
  }
  else
  { //DRAM or SRAM all other transistors
    dt = &(g_tp.peri_global);
  }
  return nWidth*dt->I_g_on_n;
}

double cmos_Ig_p(
    double pWidth,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr)
{
  TechnologyParameter::DeviceType * dt;

  if ((!_is_dram)&&(_is_cell))
  { //SRAM cell access transistor
    dt = &(g_tp.sram_cell);
  }
  else if ((_is_dram)&&(_is_wl_tr))
  { //DRAM wordline transistor
    dt = &(g_tp.dram_wl);
  }
  else
  { //DRAM or SRAM all other transistors
    dt = &(g_tp.peri_global);
  }
  return pWidth*dt->I_g_on_p;
}

double cmos_Isub_leakage(
    double nWidth,
    double pWidth,
    int    fanin,
    enum Gate_type g_type,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr,
    enum Half_net_topology topo)
{
	assert (fanin>=1);
	double nmos_leak = simplified_nmos_leakage(nWidth, _is_dram, _is_cell, _is_wl_tr);
	double pmos_leak = simplified_pmos_leakage(pWidth, _is_dram, _is_cell, _is_wl_tr);
    double Isub=0;
    int    num_states;
    int    num_off_tx;

    num_states = int(pow(2.0, fanin));

    switch (g_type)
    {
    case nmos:
    	if (fanin==1)
    	{
    		Isub = nmos_leak/num_states;
    	}
    	else
    	{
    		if (topo==parallel)
    		{
    			Isub=nmos_leak*fanin/num_states; //only when all tx are off, leakage power is non-zero. The possibility of this state is 1/num_states
    		}
    		else
    		{
    			for (num_off_tx=1; num_off_tx<=fanin; num_off_tx++) //when num_off_tx ==0 there is no leakage power
    			{
    				//Isub += nmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*(factorial(fanin)/(factorial(fanin, num_off_tx)*factorial(num_off_tx)));
    				Isub += nmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*combination(fanin, num_off_tx);
    			}
    			Isub /=num_states;
    		}

    	}
    	break;
    case pmos:
    	if (fanin==1)
    	{
    		Isub = pmos_leak/num_states;
    	}
    	else
    	{
    		if (topo==parallel)
    		{
    			Isub=pmos_leak*fanin/num_states; //only when all tx are off, leakage power is non-zero. The possibility of this state is 1/num_states
    		}
    		else
    		{
    			for (num_off_tx=1; num_off_tx<=fanin; num_off_tx++) //when num_off_tx ==0 there is no leakage power
    			{
    				//Isub += pmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*(factorial(fanin)/(factorial(fanin, num_off_tx)*factorial(num_off_tx)));
    				Isub += pmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*combination(fanin, num_off_tx);
    			}
    			Isub /=num_states;
    		}

    	}
    	break;
    case inv:
    	Isub = (nmos_leak + pmos_leak)/2;
    	break;
    case nand:
    	Isub += fanin*pmos_leak;//the pullup network
    	for (num_off_tx=1; num_off_tx<=fanin; num_off_tx++) // the pulldown network
    	{
    		//Isub += nmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*(factorial(fanin)/(factorial(fanin, num_off_tx)*factorial(num_off_tx)));
            Isub += nmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*combination(fanin, num_off_tx);
    	}
    	Isub /=num_states;
    	break;
    case nor:
    	for (num_off_tx=1; num_off_tx<=fanin; num_off_tx++) // the pullup network
    	{
    		//Isub += pmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*(factorial(fanin)/(factorial(fanin, num_off_tx)*factorial(num_off_tx)));
    		Isub += pmos_leak*pow(UNI_LEAK_STACK_FACTOR,(num_off_tx-1))*combination(fanin, num_off_tx);
    	}
    	Isub += fanin*nmos_leak;//the pulldown network
    	Isub /=num_states;
    	break;
    case tri:
    	Isub += (nmos_leak + pmos_leak)/2;//enabled
    	Isub += nmos_leak*UNI_LEAK_STACK_FACTOR; //disabled upper bound of leakage power
    	Isub /=2;
    	break;
    case tg:
    	Isub = (nmos_leak + pmos_leak)/2;
    	break;
    default:
    	assert(0);
    	break;
	  }

    return Isub;
}


double cmos_Ig_leakage(
    double nWidth,
    double pWidth,
    int    fanin,
    enum Gate_type g_type,
    bool _is_dram,
    bool _is_cell,
    bool _is_wl_tr,
    enum Half_net_topology topo)
{
	assert (fanin>=1);
		double nmos_leak = cmos_Ig_n(nWidth, _is_dram, _is_cell, _is_wl_tr);
		double pmos_leak = cmos_Ig_p(pWidth, _is_dram, _is_cell, _is_wl_tr);
	    double Ig_on=0;
	    int    num_states;
	    int    num_on_tx;

	    num_states = int(pow(2.0, fanin));

	    switch (g_type)
	    {
	    case nmos:
	    	if (fanin==1)
	    	{
	    		Ig_on = nmos_leak/num_states;
	    	}
	    	else
	    	{
	    		if (topo==parallel)
	    		{
	    	    	for (num_on_tx=1; num_on_tx<=fanin; num_on_tx++)
	    	        {
	    	    		Ig_on += nmos_leak*combination(fanin, num_on_tx)*num_on_tx;
	    	    	}
	    		}
	    		else
	    		{
	    			Ig_on += nmos_leak * fanin;//pull down network when all TXs are on.
	    		    //num_on_tx is the number of on tx
	    			for (num_on_tx=1; num_on_tx<fanin; num_on_tx++)//when num_on_tx=[1,n-1]
	    			{
	    				Ig_on += nmos_leak*combination(fanin, num_on_tx)*num_on_tx/2;//TODO: this is a approximation now, a precise computation will be very complicated.
	    			}
	    			Ig_on /=num_states;
	    		}
	    	}
	    	break;
	    case pmos:
	    	if (fanin==1)
	    	{
	    		Ig_on = pmos_leak/num_states;
	    	}
	    	else
	    	{
	    		if (topo==parallel)
    		    {
    	    	  for (num_on_tx=1; num_on_tx<=fanin; num_on_tx++)
    	          {
    	    		  Ig_on += pmos_leak*combination(fanin, num_on_tx)*num_on_tx;
    	    	  }
    		    }
    		    else
    		    {
    			  Ig_on += pmos_leak * fanin;//pull down network when all TXs are on.
    		      //num_on_tx is the number of on tx
    			  for (num_on_tx=1; num_on_tx<fanin; num_on_tx++)//when num_on_tx=[1,n-1]
    			  {
    				  Ig_on += pmos_leak*combination(fanin, num_on_tx)*num_on_tx/2;//TODO: this is a approximation now, a precise computation will be very complicated.
    			  }
	    		  Ig_on /=num_states;
	    	    }
	    	}
	    	break;

	    case inv:
	    	Ig_on = (nmos_leak + pmos_leak)/2;
	    	break;
	    case nand:
	    	//pull up network
	    	for (num_on_tx=1; num_on_tx<=fanin; num_on_tx++)//when num_on_tx=[1,n]
	        {
	    		Ig_on += pmos_leak*combination(fanin, num_on_tx)*num_on_tx;
	    	}

	    	//pull down network
	    	Ig_on += nmos_leak * fanin;//pull down network when all TXs are on.
	    	//num_on_tx is the number of on tx
	    	for (num_on_tx=1; num_on_tx<fanin; num_on_tx++)//when num_on_tx=[1,n-1]
	    	{
	    		Ig_on += nmos_leak*combination(fanin, num_on_tx)*num_on_tx/2;//TODO: this is a approximation now, a precise computation will be very complicated.
	    	}
	    	Ig_on /=num_states;
	    	break;
	    case nor:
	    	// num_on_tx is the number of on tx in pull up network
	    	Ig_on += pmos_leak * fanin;//pull up network when all TXs are on.
	    	for (num_on_tx=1; num_on_tx<fanin; num_on_tx++)
	    	{
	    		Ig_on += pmos_leak*combination(fanin, num_on_tx)*num_on_tx/2;

	    	}
	    	//pull down network
	    	for (num_on_tx=1; num_on_tx<=fanin; num_on_tx++)//when num_on_tx=[1,n]
	        {
	    		Ig_on += nmos_leak*combination(fanin, num_on_tx)*num_on_tx;
	    	}
	    	Ig_on /=num_states;
	    	break;
	    case tri:
	    	Ig_on += (2*nmos_leak + 2*pmos_leak)/2;//enabled
	    	Ig_on += (nmos_leak + pmos_leak)/2; //disabled upper bound of leakage power
	    	Ig_on /=2;
	    	break;
	    case tg:
	    	Ig_on = (nmos_leak + pmos_leak)/2;
	    	break;
	    default:
	    	assert(0);
	    	break;
		  }

	    return Ig_on;
}

double shortcircuit_simple(
    double vt,
    double velocity_index,
    double c_in,
    double c_out,
    double w_nmos,
    double w_pmos,
    double i_on_n,
    double i_on_p,
    double i_on_n_in,
    double i_on_p_in,
    double vdd)
{

	double p_short_circuit, p_short_circuit_discharge, p_short_circuit_charge, p_short_circuit_discharge_low, p_short_circuit_discharge_high, p_short_circuit_charge_low, p_short_circuit_charge_high; //this is actually energy
	double fo_n, fo_p, fanout, beta_ratio, vt_to_vdd_ratio;

	fo_n	= i_on_n/i_on_n_in;
	fo_p	= i_on_p/i_on_p_in;
	fanout	= c_out/c_in;
	beta_ratio = i_on_p/i_on_n;
	vt_to_vdd_ratio = vt/vdd;

	//p_short_circuit_discharge_low 	= 10/3*(pow(0.5-vt_to_vdd_ratio,3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_p*fo_p/fanout/beta_ratio;
	p_short_circuit_discharge_low 	= 10/3*(pow(((vdd-vt)-vt_to_vdd_ratio),3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_p*fo_p/fanout/beta_ratio;
	p_short_circuit_charge_low 		= 10/3*(pow(((vdd-vt)-vt_to_vdd_ratio),3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_n*fo_n/fanout*beta_ratio;
//	double t1, t2, t3, t4, t5;
//	t1=pow(((vdd-vt)-vt_to_vdd_ratio),3);
//	t2=pow(velocity_index,2.0);
//	t3=pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio);
//	t4=t1/t2/t3;
//	cout <<t1<<"t1\n"<<t2<<"t2\n"<<t3<<"t3\n"<<t4<<"t4\n"<<fanout<<endl;

	p_short_circuit_discharge_high 	= pow(((vdd-vt)-vt_to_vdd_ratio),1.5)*c_in*vdd*vdd*fo_p/10/pow(2, 3*vt_to_vdd_ratio+2*velocity_index);
	p_short_circuit_charge_high 	= pow(((vdd-vt)-vt_to_vdd_ratio),1.5)*c_in*vdd*vdd*fo_n/10/pow(2, 3*vt_to_vdd_ratio+2*velocity_index);

//	t1=pow(((vdd-vt)-vt_to_vdd_ratio),1.5);
//	t2=pow(2, 3*vt_to_vdd_ratio+2*velocity_index);
//	t3=t1/t2;
//	cout <<t1<<"t1\n"<<t2<<"t2\n"<<t3<<"t3\n"<<t4<<"t4\n"<<fanout<<endl;
//	p_short_circuit_discharge = 1.0/(1.0/p_short_circuit_discharge_low + 1.0/p_short_circuit_discharge_high);
//	p_short_circuit_charge = 1/(1/p_short_circuit_charge_low + 1/p_short_circuit_charge_high); //harmmoic mean cannot be applied simple formulas.

	p_short_circuit_discharge = p_short_circuit_discharge_low;
	p_short_circuit_charge = p_short_circuit_charge_low;
	p_short_circuit = (p_short_circuit_discharge + p_short_circuit_charge)/2;

  return (p_short_circuit);
}

double shortcircuit(
    double vt,
    double velocity_index,
    double c_in,
    double c_out,
    double w_nmos,
    double w_pmos,
    double i_on_n,
    double i_on_p,
    double i_on_n_in,
    double i_on_p_in,
    double vdd)
{

	double p_short_circuit=0, p_short_circuit_discharge;//, p_short_circuit_charge, p_short_circuit_discharge_low, p_short_circuit_discharge_high, p_short_circuit_charge_low, p_short_circuit_charge_high; //this is actually energy
	double fo_n, fo_p, fanout, beta_ratio, vt_to_vdd_ratio;
	double f_alpha, k_v, e, g_v_alpha, h_v_alpha;

	fo_n		= i_on_n/i_on_n_in;
	fo_p		= i_on_p/i_on_p_in;
	fanout		= 1;
	beta_ratio 	= i_on_p/i_on_n;
	vt_to_vdd_ratio = vt/vdd;
	e 			= 	2.71828;
	f_alpha		=	1/(velocity_index+2) -velocity_index/(2*(velocity_index+3)) +velocity_index/(velocity_index+4)*(velocity_index/2-1);
	k_v			=	0.9/0.8+(vdd-vt)/0.8*log(10*(vdd-vt)/e);
	g_v_alpha	=	(velocity_index + 1)*pow((1-velocity_index),velocity_index)*pow((1-velocity_index),velocity_index/2)/f_alpha/pow((1-velocity_index-velocity_index),(velocity_index/2+velocity_index+2));
	h_v_alpha	=   pow(2, velocity_index)*(velocity_index+1)*pow((1-velocity_index),velocity_index)/pow((1-velocity_index-velocity_index),(velocity_index+1));

	//p_short_circuit_discharge_low 	= 10/3*(pow(0.5-vt_to_vdd_ratio,3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_p*fo_p/fanout/beta_ratio;
//	p_short_circuit_discharge_low 	= 10/3*(pow(((vdd-vt)-vt_to_vdd_ratio),3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_p*fo_p/fanout/beta_ratio;
//	p_short_circuit_charge_low 		= 10/3*(pow(((vdd-vt)-vt_to_vdd_ratio),3.0)/pow(velocity_index,2.0)/pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio))*c_in*vdd*vdd*fo_n*fo_n/fanout*beta_ratio;
//	double t1, t2, t3, t4, t5;
//	t1=pow(((vdd-vt)-vt_to_vdd_ratio),3);
//	t2=pow(velocity_index,2.0);
//	t3=pow(2.0,3*vt_to_vdd_ratio*vt_to_vdd_ratio);
//	t4=t1/t2/t3;
//
//	cout <<t1<<"t1\n"<<t2<<"t2\n"<<t3<<"t3\n"<<t4<<"t4\n"<<fanout<<endl;
//
//
//	p_short_circuit_discharge_high 	= pow(((vdd-vt)-vt_to_vdd_ratio),1.5)*c_in*vdd*vdd*fo_p/10/pow(2, 3*vt_to_vdd_ratio+2*velocity_index);
//	p_short_circuit_charge_high 	= pow(((vdd-vt)-vt_to_vdd_ratio),1.5)*c_in*vdd*vdd*fo_n/10/pow(2, 3*vt_to_vdd_ratio+2*velocity_index);
//
//	p_short_circuit_discharge = 1.0/(1.0/p_short_circuit_discharge_low + 1.0/p_short_circuit_discharge_high);
//	p_short_circuit_charge = 1/(1/p_short_circuit_charge_low + 1/p_short_circuit_charge_high);
//
//	p_short_circuit = (p_short_circuit_discharge + p_short_circuit_charge)/2;
//
//	p_short_circuit = p_short_circuit_discharge;

	p_short_circuit_discharge = k_v*vdd*vdd*c_in*fo_p*fo_p/((vdd-vt)*g_v_alpha*fanout*beta_ratio/2/k_v + h_v_alpha*fo_p);
  return (p_short_circuit);
}

}
