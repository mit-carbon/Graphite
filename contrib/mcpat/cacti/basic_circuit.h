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



#ifndef __BASIC_CIRCUIT_H__
#define __BASIC_CIRCUIT_H__

#include "const.h"
#include "cacti_interface.h"

using namespace std;

namespace McPAT
{

#define UNI_LEAK_STACK_FACTOR 0.43

int powers (int base, int n);
bool is_pow2(int64_t val);
uint32_t _log2(uint64_t num);
int factorial(int n, int m = 1);
int combination(int n, int m);

//#define DBG
#ifdef DBG
    #define PRINTDW(a);\
    a;
#else
    #define PRINTDW(a);\

#endif


enum Wire_placement {
    outside_mat,
    inside_mat,
    local_wires
};



enum Htree_type {
    Add_htree,
    Data_in_htree,
    Data_out_htree,
    Search_in_htree,
    Search_out_htree,
};

enum Gate_type {
    nmos,
    pmos,
	inv,
    nand,
    nor,
    tri,
    tg
};

enum Half_net_topology {
    parallel,
    series
};

double logtwo (double x);

double gate_C(
    double width,
    double wirelength,
    bool _is_dram = false,
    bool _is_sram = false,
    bool _is_wl_tr = false);

double gate_C_pass(
    double width,
    double wirelength,
    bool   _is_dram = false,
    bool   _is_sram = false,
    bool   _is_wl_tr = false);

double drain_C_(
    double width,
    int nchannel,
    int stack,
    int next_arg_thresh_folding_width_or_height_cell,
    double fold_dimension,
    bool _is_dram = false,
    bool _is_sram = false,
    bool _is_wl_tr = false);

double tr_R_on(
    double width,
    int nchannel,
    int stack,
    bool _is_dram = false,
    bool _is_sram = false,
    bool _is_wl_tr = false);

double R_to_w(
    double res,
    int nchannel,
    bool _is_dram = false,
    bool _is_sram = false,
    bool _is_wl_tr = false);

double horowitz (
    double inputramptime,
    double tf,
    double vs1,
    double vs2,
    int rise);

double pmos_to_nmos_sz_ratio(
    bool _is_dram = false,
    bool _is_wl_tr = false);

double simplified_nmos_leakage(
    double nwidth,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr = false);

double simplified_pmos_leakage(
    double pwidth,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr = false);


double cmos_Ileak(
    double nWidth,
    double pWidth,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr = false);

double cmos_Ig_n(
    double nWidth,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr= false);

double cmos_Ig_p(
    double pWidth,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr= false);


double cmos_Isub_leakage(
    double nWidth,
    double pWidth,
    int    fanin,
    enum Gate_type g_type,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr = false,
    enum Half_net_topology topo = series);

double cmos_Ig_leakage(
    double nWidth,
    double pWidth,
    int    fanin,
    enum Gate_type g_type,
    bool _is_dram = false,
    bool _is_cell = false,
    bool _is_wl_tr = false,
    enum Half_net_topology topo = series);

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
    double vdd);

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
    double vdd);
//set power point product mask; strictly speaking this is not real point product
inline void set_pppm(
	double * pppv,
	double a=1,
    double b=1,
    double c=1,
    double d=1
    ){
		pppv[0]= a;
		pppv[1]= b;
		pppv[2]= c;
		pppv[3]= d;

}

inline void set_sppm(
	double * sppv,
	double a=1,
    double b=1,
    double c=1,
    double d=1
    ){
		sppv[0]= a;
		sppv[1]= b;
		sppv[2]= c;
}

}

#endif
