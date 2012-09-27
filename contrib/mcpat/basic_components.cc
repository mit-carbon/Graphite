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

#include "basic_components.h"
#include <iostream>
#include <assert.h>
#include <cmath>

namespace McPAT
{

double longer_channel_device_reduction(
		enum Device_ty device_ty,
		enum Core_type core_ty)
{

	double longer_channel_device_percentage_core;
	double longer_channel_device_percentage_uncore;
	double longer_channel_device_percentage_llc;

	double long_channel_device_reduction;

	longer_channel_device_percentage_llc    = 1.0;
	longer_channel_device_percentage_uncore = 0.82;
	if (core_ty==OOO)
	{
		longer_channel_device_percentage_core   = 0.56;//0.54 Xeon Tulsa //0.58 Nehelam
		//longer_channel_device_percentage_uncore = 0.76;//0.85 Nehelam

	}
	else
	{
		longer_channel_device_percentage_core   = 0.8;//0.8;//Niagara
		//longer_channel_device_percentage_uncore = 0.9;//Niagara
	}

	if (device_ty==Core_device)
	{
		long_channel_device_reduction = (1- longer_channel_device_percentage_core)
		+ longer_channel_device_percentage_core * g_tp.peri_global.long_channel_leakage_reduction;
	}
	else if (device_ty==Uncore_device)
	{
		long_channel_device_reduction = (1- longer_channel_device_percentage_uncore)
		+ longer_channel_device_percentage_uncore * g_tp.peri_global.long_channel_leakage_reduction;
	}
	else if (device_ty==LLC_device)
	{
		long_channel_device_reduction = (1- longer_channel_device_percentage_llc)
		+ longer_channel_device_percentage_llc * g_tp.peri_global.long_channel_leakage_reduction;
	}
	else
	{
		cout<<"unknown device category"<<endl;
		exit(0);
	}

	return long_channel_device_reduction;
}

statsComponents operator+(const statsComponents & x, const statsComponents & y)
{
	statsComponents z;

	z.access = x.access + y.access;
	z.hit    = x.hit + y.hit;
	z.miss   = x.miss  + y.miss;

	return z;
}

statsComponents operator*(const statsComponents & x, double const * const y)
{
	statsComponents z;

	z.access = x.access*y[0];
	z.hit    = x.hit*y[1];
	z.miss   = x.miss*y[2];

	return z;
}

statsDef operator+(const statsDef & x, const statsDef & y)
{
	statsDef z;

	z.readAc   = x.readAc  + y.readAc;
	z.writeAc  = x.writeAc + y.writeAc;
	z.searchAc  = x.searchAc + y.searchAc;
	return z;
}

statsDef operator*(const statsDef & x, double const * const y)
{
	statsDef z;

	z.readAc   = x.readAc*y;
	z.writeAc  = x.writeAc*y;
	z.searchAc  = x.searchAc*y;
	return z;
}

}
