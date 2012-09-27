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

#ifndef ARRAY_H_
#define ARRAY_H_

#include "basic_components.h"
#include "const.h"
#include "cacti_interface.h"
#include "parameter.h"
#include "component.h"
#include <iostream>
#include <string>

using namespace std;

namespace McPAT
{

class ArrayST :public Component{
 public:
  ArrayST(){};
  ArrayST(const InputParameter *configure_interface, string _name, enum Device_ty device_ty_, bool opt_local_=true, enum Core_type core_ty_=Inorder,  bool _is_default=true);

  InputParameter l_ip;
  string         name;
  enum Device_ty device_ty;
  bool opt_local;
  enum Core_type core_ty;
  bool           is_default;
  uca_org_t      local_result;

  statsDef       tdp_stats;
  statsDef       rtp_stats;
  statsDef       stats_t;
  powerDef       power_t;

  virtual void optimize_array();
  virtual void compute_base_power();
  virtual ~ArrayST();
};

class InstCache :public Component{
public:
  ArrayST* caches;
  ArrayST* missb;
  ArrayST* ifb;
  ArrayST* prefetchb;
  powerDef power_t;//temp value holder for both (max) power and runtime power
  InstCache(){caches=0;missb=0;ifb=0;prefetchb=0;};
  ~InstCache(){
	  if (caches)    {//caches->local_result.cleanup();
					  delete caches; caches=0;}
	  if (missb)     {//missb->local_result.cleanup();
					  delete missb; missb=0;}
	  if (ifb)       {//ifb->local_result.cleanup();
					  delete ifb; ifb=0;}
	  if (prefetchb) {//prefetchb->local_result.cleanup();
					  delete prefetchb; prefetchb=0;}
   };
};

class DataCache :public InstCache{
public:
  ArrayST* wbb;
  DataCache(){wbb=0;};
  ~DataCache(){
	  if (wbb) {//wbb->local_result.cleanup();
				delete wbb; wbb=0;}
   };
};

}

#endif /* TLB_H_ */
