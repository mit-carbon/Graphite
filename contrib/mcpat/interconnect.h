/*****************************************************************************
 *                                McPAT
 *                      SOFTWARE LICENSE AGREEMENT
 *            Copyright 2009 Hewlett-Packard Development Company, L.P.
 *                          All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Any User of the software ("User"), by accessing and using it, agrees to the
 * terms and conditions set forth herein, and hereby grants back to Hewlett-
 * Packard Development Company, L.P. and its affiliated companies ("HP") a
 * non-exclusive, unrestricted, royalty-free right and license to copy,
 * modify, distribute copies, create derivate works and publicly display and
 * use, any changes, modifications, enhancements or extensions made to the
 * software by User, including but not limited to those affording
 * compatibility with other hardware or software, but excluding pre-existing
 * software applications that may incorporate the software.  User further
 * agrees to use its best efforts to inform HP of any such changes,
 * modifications, enhancements or extensions.
 *
 * Correspondence should be provided to HP at:
 *
 * Director of Intellectual Property Licensing
 * Office of Strategy and Technology
 * Hewlett-Packard Company
 * 1501 Page Mill Road
 * Palo Alto, California  94304
 *
 * The software may be further distributed by User (but not offered for
 * sale or transferred for compensation) to third parties, under the
 * condition that such third parties agree to abide by the terms and
 * conditions of this license.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITH ANY AND ALL ERRORS AND DEFECTS
 * AND USER ACKNOWLEDGES THAT THE SOFTWARE MAY CONTAIN ERRORS AND DEFECTS.
 * HP DISCLAIMS ALL WARRANTIES WITH REGARD TO THE SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL
 * HP BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 *
 ***************************************************************************/


#ifndef __INTERCONNECT_H__
#define __INTERCONNECT_H__

#include "basic_circuit.h"
#include "basic_components.h"
#include "component.h"
#include "parameter.h"
#include "assert.h"
#include "subarray.h"
#include "cacti_interface.h"
#include "wire.h"

namespace McPAT
{

// leakge power includes entire htree in a bank (when uca_tree == false)
// leakge power includes only part to one bank when uca_tree == true

class interconnect : public Component
{
  public:
    interconnect(
        string  name_,
        enum Device_ty device_ty_,
    	double base_w, double base_h, int data_w, double len,
        const InputParameter *configure_interface, int start_wiring_level_,
        bool pipelinable_ = false,
        double route_over_perc_ =0.5,
        bool opt_local_=true,
        enum Core_type core_ty_=Inorder,
        enum Wire_type wire_model=Global,
        double width_s=1.0, double space_s=1.0,
        TechnologyParameter::DeviceType *dt = &(g_tp.peri_global)
		);

    ~interconnect() {};

    void compute();
	string   name;
	enum Device_ty device_ty;
    double in_rise_time, out_rise_time;
	InputParameter l_ip;
	uca_org_t local_result;
    Area no_device_under_wire_area;
    void set_in_rise_time(double rt)
    {
      in_rise_time = rt;
    }

    double max_unpipelined_link_delay;
    powerDef power_bit;

    double wire_bw;
    double init_wire_bw;  // bus width at root
    double base_width;
    double base_height;
    int data_width;
    enum Wire_type wt;
    double width_scaling, space_scaling;
    int start_wiring_level;
    double length;
    double min_w_nmos;
    double min_w_pmos;
    double latency, throughput;
    bool  latency_overflow;
    bool  throughput_overflow;
    double  interconnect_latency;
    double  interconnect_throughput;
    bool opt_local;
    enum Core_type core_ty;
    bool pipelinable;
    double route_over_perc;
    int  num_pipe_stages;

  private:
    TechnologyParameter::DeviceType *deviceType;

};

}

#endif

