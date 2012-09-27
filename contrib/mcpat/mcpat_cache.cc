/*****************************************************************************
 * Graphite-McPAT Cache Interface
 ***************************************************************************/

// [graphite]

#include <string.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <string.h>
#include <cmath>
#include <assert.h>
#include <fstream>
#include "parameter.h"
#include "array.h"
#include "const.h"
#include "basic_circuit.h"
#include "XML_Parse.h"
#include "processor.h"
#include "version.h"
#include "mcpat_cache.h"

namespace McPAT
{

McPATCache::McPATCache(ParseXML *XML_interface)
:XML(XML_interface)//TODO: using one global copy may have problems.
{
  /*
   *  placement and routing overhead is 10%, core scales worse than cache 40% is accumulated from 90 to 22nm
   *  There is no point to have heterogeneous memory controller on chip,
   *  thus McPAT only support homogeneous memory controllers.
   */
  int i;
  double pppm_t[4]    = {1,1,1,1};
  set_proc_param();

  if (procdynp.homoL2)
     numL2 = procdynp.numL2==0? 0:1;
  else
     numL2 = procdynp.numL2;

  if (!XML->sys.Private_L2)
  {
  if (numL2 >0)
     for (i = 0;i < numL2; i++)
     {
        l2array.push_back(new SharedCache(XML,i, &interface_ip));
        if (procdynp.homoL2){
           l2.area.set_area(l2.area.get_area() + l2array[i]->area.get_area()*procdynp.numL2);
           area.set_area(area.get_area() + l2.area.get_area());//placement and routing overhead is 10%, l2 scales worse than cache 40% is accumulated from 90 to 22nm
        }
        else{
           l2.area.set_area(l2.area.get_area() + l2array[i]->area.get_area());
           area.set_area(area.get_area() + l2array[i]->area.get_area());//placement and routing overhead is 10%, l2 scales worse than cache 40% is accumulated from 90 to 22nm
        }
     }
  }
}

//---------------------------------------------------------------------------
// [graphite] Compute Energy
//---------------------------------------------------------------------------
void McPATCache::computeEnergy()
{
  /*
   *  placement and routing overhead is 10%, core scales worse than cache 40% is accumulated from 90 to 22nm
   *  There is no point to have heterogeneous memory controller on chip,
   *  thus McPAT only support homogeneous memory controllers.
   */
  int i;
  double pppm_t[4]    = {1,1,1,1};

  //--------------------------------------
  // Set Numbers of Components
  //--------------------------------------
  if (procdynp.homoL2)
     numL2 = procdynp.numL2==0? 0:1;
  else
     numL2 = procdynp.numL2;

  //--------------------------------------
  // Compute Energy of Components
  //--------------------------------------

  // McPAT Cache
  power.reset();
  rt_power.reset();

  // L2 Caches
  l2.power.reset();
  l2.rt_power.reset();
  if (!XML->sys.Private_L2)
  {
  if (numL2 >0)
     for (i = 0;i < numL2; i++)
     {
        l2array[i]->power.reset();
        l2array[i]->rt_power.reset();
        l2array[i]->computeEnergy();
        l2array[i]->computeEnergy(false);
        if (procdynp.homoL2){
           set_pppm(pppm_t,l2array[i]->cachep.clockRate*procdynp.numL2, procdynp.numL2,procdynp.numL2,procdynp.numL2);
           l2.power = l2.power + l2array[i]->power*pppm_t;
           set_pppm(pppm_t,1/l2array[i]->cachep.executionTime, procdynp.numL2,procdynp.numL2,procdynp.numL2);
           l2.rt_power = l2.rt_power + l2array[i]->rt_power*pppm_t;
           power = power  + l2.power;
           rt_power = rt_power  + l2.rt_power;
        }
        else{
           set_pppm(pppm_t,l2array[i]->cachep.clockRate, 1, 1, 1);
           l2.power = l2.power + l2array[i]->power*pppm_t;
           power = power  + l2array[i]->power*pppm_t;;
           set_pppm(pppm_t,1/l2array[i]->cachep.executionTime, 1, 1, 1);
           l2.rt_power = l2.rt_power + l2array[i]->rt_power*pppm_t;
           rt_power = rt_power  + l2array[i]->rt_power*pppm_t;
        }
     }
  }
}

//---------------------------------------------------------------------------
// Print Device Type
//---------------------------------------------------------------------------
void McPATCache::displayDeviceType(int device_type_, uint32_t indent)
{
   string indent_str(indent, ' ');

   switch ( device_type_ ) {

     case 0 :
        cout <<indent_str<<"Device Type= "<<"ITRS high performance device type"<<endl;
       break;
     case 1 :
        cout <<indent_str<<"Device Type= "<<"ITRS low standby power device type"<<endl;
       break;
     case 2 :
        cout <<indent_str<<"Device Type= "<<"ITRS low operating power device type"<<endl;
       break;
     case 3 :
        cout <<indent_str<<"Device Type= "<<"LP-DRAM device type"<<endl;
       break;
     case 4 :
        cout <<indent_str<<"Device Type= "<<"COMM-DRAM device type"<<endl;
       break;
     default :
        {
           cout <<indent_str<<"Unknown Device Type"<<endl;
           exit(0);
        }
   }
}

//---------------------------------------------------------------------------
// Print Interconnect Type
//---------------------------------------------------------------------------
void McPATCache::displayInterconnectType(int interconnect_type_, uint32_t indent)
{
   string indent_str(indent, ' ');

   switch ( interconnect_type_ ) {

     case 0 :
        cout <<indent_str<<"Interconnect metal projection= "<<"aggressive interconnect technology projection"<<endl;
       break;
     case 1 :
        cout <<indent_str<<"Interconnect metal projection= "<<"conservative interconnect technology projection"<<endl;
       break;
     default :
        {
           cout <<indent_str<<"Unknown Interconnect Projection Type"<<endl;
           exit(0);
        }
   }
}

//---------------------------------------------------------------------------
// Print Energy from McPAT Cache
//---------------------------------------------------------------------------
void McPATCache::displayEnergy(uint32_t indent, int plevel, bool is_tdp)
{
   int i;
   bool long_channel = XML->sys.longer_channel_device;
   string indent_str(indent, ' ');
   string indent_str_next(indent+2, ' ');
   if (is_tdp)
   {

      if (plevel<5)
      {
         cout<<"\nMcPAT (version "<< VER_MAJOR <<"."<< VER_MINOR
               << " of " << VER_UPDATE << ") results (current print level is "<< plevel
         <<", please increase print level to see the details in components): "<<endl;
      }
      else
      {
         cout<<"\nMcPAT (version "<< VER_MAJOR <<"."<< VER_MINOR
                        << " of " << VER_UPDATE << ") results  (current print level is 5)"<< endl;
      }
      cout <<"*****************************************************************************************"<<endl;
      cout <<indent_str<<"Technology "<<XML->sys.core_tech_node<<" nm"<<endl;
      //cout <<indent_str<<"Device Type= "<<XML->sys.device_type<<endl;
      if (long_channel)
         cout <<indent_str<<"Using Long Channel Devices When Appropriate"<<endl;
      //cout <<indent_str<<"Interconnect metal projection= "<<XML->sys.interconnect_projection_type<<endl;
      displayInterconnectType(XML->sys.interconnect_projection_type, indent);
      cout <<indent_str<<"Core clock Rate(MHz) "<<XML->sys.core[0].clock_rate<<endl;
      cout <<endl;
      cout <<"*****************************************************************************************"<<endl;
      if (!XML->sys.Private_L2)
      {
         if (numL2 >0){
            cout <<indent_str<<"Total L2s: "<<endl;
            displayDeviceType(XML->sys.L2[0].device_type,indent);
            cout << indent_str_next << "Area = " << l2.area.get_area()*1e-6<< " mm^2" << endl;
            cout << indent_str_next << "Peak Dynamic = " << l2.power.readOp.dynamic << " W" << endl;
            cout << indent_str_next << "Subthreshold Leakage = "
            << (long_channel? l2.power.readOp.longer_channel_leakage:l2.power.readOp.leakage) <<" W" << endl;
            //cout << indent_str_next << "Subthreshold Leakage = " << l2.power.readOp.longer_channel_leakage <<" W" << endl;
            cout << indent_str_next << "Gate Leakage = " << l2.power.readOp.gate_leakage << " W" << endl;
            cout << indent_str_next << "Runtime Dynamic = " << l2.rt_power.readOp.dynamic << " W" << endl;
            cout <<endl;
         }
      }
      cout <<"*****************************************************************************************"<<endl;
      if (plevel >1)
      {
         if (!XML->sys.Private_L2)
         {
            for (i = 0;i < numL2; i++)
            {
               l2array[i]->displayEnergy(indent+4,is_tdp);
               cout <<"*****************************************************************************************"<<endl;
            }
         }
      }
   }
   else
   {
   }
}

//---------------------------------------------------------------------------
// Set McPAT Cache Parameters
//---------------------------------------------------------------------------
void McPATCache::set_proc_param()
{
   bool debug = false;

   procdynp.homoL2   = bool(debug?1:XML->sys.homogeneous_L2s);
   procdynp.numL2   = XML->sys.number_of_L2s;

   /* Basic parameters*/
   interface_ip.data_arr_ram_cell_tech_type    = debug?0:XML->sys.device_type;
   interface_ip.data_arr_peri_global_tech_type = debug?0:XML->sys.device_type;
   interface_ip.tag_arr_ram_cell_tech_type     = debug?0:XML->sys.device_type;
   interface_ip.tag_arr_peri_global_tech_type  = debug?0:XML->sys.device_type;

   interface_ip.ic_proj_type     = debug?0:XML->sys.interconnect_projection_type;
   interface_ip.delay_wt                = 100;//Fixed number, make sure timing can be satisfied.
   interface_ip.area_wt                 = 0;//Fixed number, This is used to exhaustive search for individual components.
   interface_ip.dynamic_power_wt        = 100;//Fixed number, This is used to exhaustive search for individual components.
   interface_ip.leakage_power_wt        = 0;
   interface_ip.cycle_time_wt           = 0;

   interface_ip.delay_dev                = 10000;//Fixed number, make sure timing can be satisfied.
   interface_ip.area_dev                 = 10000;//Fixed number, This is used to exhaustive search for individual components.
   interface_ip.dynamic_power_dev        = 10000;//Fixed number, This is used to exhaustive search for individual components.
   interface_ip.leakage_power_dev        = 10000;
   interface_ip.cycle_time_dev           = 10000;

   interface_ip.ed                       = 2;
   interface_ip.burst_len      = 1;//parameters are fixed for processor section, since memory is processed separately
   interface_ip.int_prefetch_w = 1;
   interface_ip.page_sz_bits   = 0;
   interface_ip.temp = debug?360: XML->sys.temperature;
   interface_ip.F_sz_nm         = debug?90:XML->sys.core_tech_node;//XML->sys.core_tech_node;
   interface_ip.F_sz_um         = interface_ip.F_sz_nm / 1000;

   //***********This section of code does not have real meaning, they are just to ensure all data will have initial value to prevent errors.
   //They will be overridden  during each components initialization
   interface_ip.cache_sz            =64;
   interface_ip.line_sz             = 1;
   interface_ip.assoc               = 1;
   interface_ip.nbanks              = 1;
   interface_ip.out_w               = interface_ip.line_sz*8;
   interface_ip.specific_tag        = 1;
   interface_ip.tag_w               = 64;
   interface_ip.access_mode         = 2;

   interface_ip.obj_func_dyn_energy = 0;
   interface_ip.obj_func_dyn_power  = 0;
   interface_ip.obj_func_leak_power = 0;
   interface_ip.obj_func_cycle_t    = 1;

   interface_ip.is_main_mem     = false;
   interface_ip.rpters_in_htree = true ;
   interface_ip.ver_htree_wires_over_array = 0;
   interface_ip.broadcast_addr_din_over_ver_htrees = 0;

   interface_ip.num_rw_ports        = 1;
   interface_ip.num_rd_ports        = 0;
   interface_ip.num_wr_ports        = 0;
   interface_ip.num_se_rd_ports     = 0;
   interface_ip.num_search_ports    = 1;
   interface_ip.nuca                = 0;
   interface_ip.nuca_bank_count     = 0;
   interface_ip.is_cache            =true;
   interface_ip.pure_ram            =false;
   interface_ip.pure_cam            =false;
   interface_ip.force_cache_config  =false;
   if (XML->sys.Embedded)
      {
      interface_ip.wt                  =Global_30;
      interface_ip.wire_is_mat_type = 0;
      interface_ip.wire_os_mat_type = 0;
      }
   else
      {
      interface_ip.wt                  =Global;
      interface_ip.wire_is_mat_type = 2;
      interface_ip.wire_os_mat_type = 2;
      }
   interface_ip.force_wiretype      = false;
   interface_ip.print_detail        = 1;
   interface_ip.add_ecc_b_          =true;
}

//---------------------------------------------------------------------------
// Delete McPAT Cache
//---------------------------------------------------------------------------
McPATCache::~McPATCache(){
   while (!l2array.empty())
   {
      delete l2array.back();
      l2array.pop_back();
   }
};

}
