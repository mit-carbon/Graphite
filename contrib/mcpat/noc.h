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

#ifndef NOC_H_
#define NOC_H_
#include "XML_Parse.h"
#include "logic.h"
#include "parameter.h"
#include "array.h"
#include "interconnect.h"
#include "basic_components.h"
#include "router.h"

namespace McPAT
{

class NoC :public Component {
  public:

	ParseXML *XML;
	int  ithNoC;
	InputParameter interface_ip;
	double link_len;
	double executionTime;
	double scktRatio, chip_PR_overhead, macro_PR_overhead;
	Router * router;
	interconnect * link_bus;
	NoCParam  nocdynp;
	uca_org_t local_result;
	statsDef       tdp_stats;
	statsDef       rtp_stats;
	statsDef       stats_t;
	powerDef       power_t;
	Component      link_bus_tot_per_Router;
	bool link_bus_exist;
	bool router_exist;
	string name, link_name;
	double M_traffic_pattern;
	NoC(ParseXML *XML_interface, int ithNoC_, InputParameter* interface_ip_, double M_traffic_pattern_ = 0.6,double link_len_=0);
	void set_noc_param();
	void computeEnergy(bool is_tdp=true);
	void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
	void init_link_bus(double link_len_);
	void init_router();
	void computeEnergy_link_bus(bool is_tdp=true);
	void displayEnergy_link_bus(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
	~NoC();
};

}

#endif /* NOC_H_ */
