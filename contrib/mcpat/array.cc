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

#define  GLOBALVAR
#include "area.h"
#include "decoder.h"
#include "parameter.h"
#include "array.h"
#include <iostream>
#include <math.h>
#include <assert.h>
#include "globalvar.h"

using namespace std;

namespace McPAT
{

ArrayST::ArrayST(const InputParameter *configure_interface,
		               string _name,
		               enum Device_ty device_ty_,
		               bool opt_local_,
		               enum Core_type core_ty_,
		               bool _is_default)
:l_ip(*configure_interface),
 name(_name),
 device_ty(device_ty_),
 opt_local(opt_local_),
 core_ty(core_ty_),
 is_default(_is_default)
    {

	if (l_ip.cache_sz<64) l_ip.cache_sz=64;
	l_ip.error_checking();//not only do the error checking but also fill some missing parameters
	optimize_array();

}


void ArrayST::compute_base_power()
    {
	//l_ip.out_w               =l_ip.line_sz*8;
    local_result=cacti_interface(&l_ip);

    }

void ArrayST::optimize_array()
{
	list<uca_org_t > candidate_solutions(0);
	list<uca_org_t >::iterator candidate_iter, min_dynamic_energy_iter;

	uca_org_t * temp_res = 0;
	local_result.valid=false;

	double 	throughput=l_ip.throughput, latency=l_ip.latency;
	double  area_efficiency_threshold = 20.0;
	bool 	throughput_overflow=true, latency_overflow=true;
	compute_base_power();

	if ((local_result.cycle_time - throughput) <= 1e-10 )
		throughput_overflow=false;
	if ((local_result.access_time - latency)<= 1e-10)
		latency_overflow=false;

	if (opt_for_clk && opt_local)
	{
		if (throughput_overflow || latency_overflow)
		{
			l_ip.ed=0;

			l_ip.delay_wt                = 100;//Fixed number, make sure timing can be satisfied.
			l_ip.cycle_time_wt           = 1000;

			l_ip.area_wt                 = 10;//Fixed number, This is used to exhaustive search for individual components.
			l_ip.dynamic_power_wt        = 10;//Fixed number, This is used to exhaustive search for individual components.
			l_ip.leakage_power_wt        = 10;

			l_ip.delay_dev               = 1000000;//Fixed number, make sure timing can be satisfied.
			l_ip.cycle_time_dev          = 100;

			l_ip.area_dev                = 1000000;//Fixed number, This is used to exhaustive search for individual components.
			l_ip.dynamic_power_dev       = 1000000;//Fixed number, This is used to exhaustive search for individual components.
			l_ip.leakage_power_dev       = 1000000;

			throughput_overflow=true; //Reset overflow flag before start optimization iterations
			latency_overflow=true;

			temp_res = &local_result; //Clean up the result for optimized for ED^2P
			temp_res->cleanup();
		}


		while ((throughput_overflow || latency_overflow)&&l_ip.cycle_time_dev > 10)// && l_ip.delay_dev > 10
		{
			compute_base_power();

			l_ip.cycle_time_dev-=10;//This is the time_dev to be used for next iteration

			//		from best area to worst area -->worst timing to best timing
			if ((((local_result.cycle_time - throughput) <= 1e-10 ) && (local_result.access_time - latency)<= 1e-10)||
					(local_result.data_array2->area_efficiency < area_efficiency_threshold && l_ip.assoc == 0))
			{  //if no satisfiable solution is found,the most aggressive one is left
				candidate_solutions.push_back(local_result);
				//output_data_csv(candidate_solutions.back());
				if (((local_result.cycle_time - throughput) <= 1e-10) && ((local_result.access_time - latency)<= 1e-10))
					//ensure stop opt not because of cam
				{
					throughput_overflow=false;
					latency_overflow=false;
				}

			}
			else
			{
				//TODO: whether checking the partial satisfied results too, or just change the mark???
				if ((local_result.cycle_time - throughput) <= 1e-10)
										throughput_overflow=false;
				if ((local_result.access_time - latency)<= 1e-10)
										latency_overflow=false;

				if (l_ip.cycle_time_dev > 10)
				{   //if not >10 local_result is the last result, it cannot be cleaned up
					temp_res = &local_result; //Only solutions not saved in the list need to be cleaned up
					temp_res->cleanup();
				}
			}
//			l_ip.cycle_time_dev-=10;
//			l_ip.delay_dev-=10;

		}


	if (l_ip.assoc > 0)
	{
		//For array structures except CAM and FA, Give warning but still provide a result with best timing found
		if (throughput_overflow==true)
			cout<< "Warning: " << name<<" array structure cannot satisfy throughput constraint." << endl;
		if (latency_overflow==true)
			cout<< "Warning: " << name<<" array structure cannot satisfy latency constraint." << endl;
	}

//	else
//	{
//		/*According to "Content-Addressable Memory (CAM) Circuits and
//				Architectures": A Tutorial and Survey
//				by Kostas Pagiamtzis et al.
//				CAM structures can be heavily pipelined and use look-ahead techniques,
//				therefore timing can be relaxed. But McPAT does not model the advanced
//				techniques. If continue optimizing, the area efficiency will be too low
//		*/
//		//For CAM and FA, stop opt if area efficiency is too low
//		if (throughput_overflow==true)
//			cout<< "Warning: " <<" McPAT stopped optimization on throughput for "<< name
//				<<" array structure because its area efficiency is below "<<area_efficiency_threshold<<"% " << endl;
//		if (latency_overflow==true)
//			cout<< "Warning: " <<" McPAT stopped optimization on latency for "<< name
//				<<" array structure because its area efficiency is below "<<area_efficiency_threshold<<"% " << endl;
//	}

		//double min_dynamic_energy, min_dynamic_power, min_leakage_power, min_cycle_time;
		double min_dynamic_energy=BIGNUM;
		if (candidate_solutions.empty()==false)
		{
			local_result.valid=true;
			for (candidate_iter = candidate_solutions.begin(); candidate_iter != candidate_solutions.end(); ++candidate_iter)

			{
				if (min_dynamic_energy > (candidate_iter)->power.readOp.dynamic)
				{
					min_dynamic_energy = (candidate_iter)->power.readOp.dynamic;
					min_dynamic_energy_iter = candidate_iter;
					local_result = *(min_dynamic_energy_iter);
					//TODO: since results are reordered results and l_ip may miss match. Therefore, the final output spread sheets may show the miss match.

				}
				else
				{
					candidate_iter->cleanup() ;
				}

			}


		}
	candidate_solutions.clear();
	}

	double long_channel_device_reduction = longer_channel_device_reduction(device_ty,core_ty);

	double macro_layout_overhead   = g_tp.macro_layout_overhead;
	double chip_PR_overhead        = g_tp.chip_layout_overhead;
	double total_overhead          = macro_layout_overhead*chip_PR_overhead;
	local_result.area *= total_overhead;

	//maintain constant power density
	double pppm_t[4]    = {total_overhead,1,1,total_overhead};

	double sckRation = g_tp.sckt_co_eff;
	local_result.power.readOp.dynamic *= sckRation;
	local_result.power.writeOp.dynamic *= sckRation;
	local_result.power.searchOp.dynamic *= sckRation;
	local_result.power.readOp.leakage *= l_ip.nbanks;
	local_result.power.readOp.longer_channel_leakage =
		local_result.power.readOp.leakage*long_channel_device_reduction;
	local_result.power = local_result.power* pppm_t;

	local_result.data_array2->power.readOp.dynamic *= sckRation;
	local_result.data_array2->power.writeOp.dynamic *= sckRation;
	local_result.data_array2->power.searchOp.dynamic *= sckRation;
	local_result.data_array2->power.readOp.leakage *= l_ip.nbanks;
	local_result.data_array2->power.readOp.longer_channel_leakage =
		local_result.data_array2->power.readOp.leakage*long_channel_device_reduction;
	local_result.data_array2->power = local_result.data_array2->power* pppm_t;


	if (!(l_ip.pure_cam || l_ip.pure_ram || l_ip.fully_assoc) && l_ip.is_cache)
	{
		local_result.tag_array2->power.readOp.dynamic *= sckRation;
		local_result.tag_array2->power.writeOp.dynamic *= sckRation;
		local_result.tag_array2->power.searchOp.dynamic *= sckRation;
		local_result.tag_array2->power.readOp.leakage *= l_ip.nbanks;
		local_result.tag_array2->power.readOp.longer_channel_leakage =
			local_result.tag_array2->power.readOp.leakage*long_channel_device_reduction;
		local_result.tag_array2->power = local_result.tag_array2->power* pppm_t;
	}


}

ArrayST:: ~ArrayST()
{
	local_result.cleanup();
}

}
