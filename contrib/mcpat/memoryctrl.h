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

#ifndef MEMORYCTRL_H_
#define MEMORYCTRL_H_

#include "XML_Parse.h"
#include "parameter.h"
//#include "io.h"
#include "array.h"
//#include "Undifferentiated_Core_Area.h"
#include <vector>
#include "basic_components.h"

namespace McPAT
{

class MCBackend : public Component {
  public:
    InputParameter l_ip;
    uca_org_t local_result;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
    statsDef tdp_stats;
    statsDef rtp_stats;
    statsDef stats_t;
    powerDef power_t;
    MCBackend(InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void compute();
	void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCBackend(){};
};

class MCPHY : public Component {
  public:
    InputParameter l_ip;
    uca_org_t local_result;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
    statsDef       tdp_stats;
    statsDef       rtp_stats;
    statsDef       stats_t;
    powerDef       power_t;
    MCPHY(InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void compute();
	void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCPHY(){};
};

class MCFrontEnd : public Component {
  public:
	ParseXML *XML;
	InputParameter interface_ip;
	enum MemoryCtrl_type mc_type;
	MCParam  mcp;
	selection_logic * MC_arb;
	ArrayST  * frontendBuffer;
	ArrayST  * readBuffer;
	ArrayST  * writeBuffer;

    MCFrontEnd(ParseXML *XML_interface,InputParameter* interface_ip_, const MCParam & mcp_, enum MemoryCtrl_type mc_type_);
    void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MCFrontEnd();
};

class MemoryController : public Component {
  public:
	ParseXML *XML;
	InputParameter interface_ip;
	enum MemoryCtrl_type mc_type;
    MCParam  mcp;
	MCFrontEnd * frontend;
    MCBackend * transecEngine;
    MCPHY	 * PHY;
    Pipeline * pipeLogic;

    //clock_network clockNetwork;
    MemoryController(ParseXML *XML_interface,InputParameter* interface_ip_, enum MemoryCtrl_type mc_type_);
    void set_mc_param();
    void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,int plevel = 100, bool is_tdp=true);
    ~MemoryController();
};

}

#endif /* MEMORYCTRL_H_ */
