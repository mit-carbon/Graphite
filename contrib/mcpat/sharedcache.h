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

#ifndef SHAREDCACHE_H_
#define SHAREDCACHE_H_
#include "XML_Parse.h"
#include "area.h"
#include "parameter.h"
#include "array.h"
#include "logic.h"
#include <vector>
#include "basic_components.h"

namespace McPAT
{

class SharedCache :public Component{
  public:
    ParseXML * XML;
    int ithCache;
	InputParameter interface_ip;
	enum cache_level cacheL;
    DataCache unicache;//Shared cache
    CacheDynParam cachep;
    statsDef   homenode_tdp_stats;
    statsDef   homenode_rtp_stats;
    statsDef   homenode_stats_t;
    double	   dir_overhead;
    //	cache_processor llCache,directory, directory1, inv_dir;

    //pipeline pipeLogicCache, pipeLogicDirectory;
    //clock_network				clockNetwork;
    double scktRatio, executionTime;
    //   Component L2Tot, cc, cc1, ccTot;

    SharedCache(ParseXML *XML_interface, int ithCache_, InputParameter* interface_ip_,enum cache_level cacheL_ =L2);
    void set_cache_param();
	void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,bool is_tdp=true);
    ~SharedCache(){};
};

class CCdir :public Component{
  public:
    ParseXML * XML;
    int ithCache;
	InputParameter interface_ip;
    DataCache dc;//Shared cache
    ArrayST * shadow_dir;
//	cache_processor llCache,directory, directory1, inv_dir;

    //pipeline pipeLogicCache, pipeLogicDirectory;
    //clock_network				clockNetwork;
    double scktRatio, clockRate, executionTime;
    Component L2Tot, cc, cc1, ccTot;

    CCdir(ParseXML *XML_interface, int ithCache_, InputParameter* interface_ip_);
    void computeEnergy(bool is_tdp=true);
    void displayEnergy(uint32_t indent = 0,bool is_tdp=true);
    ~CCdir();
};

}

#endif /* SHAREDCACHE_H_ */
