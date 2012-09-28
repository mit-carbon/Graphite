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

// [graphite]

#include "io.h"
#include <iostream>
#include "xmlParser.h"
#include "XML_Parse.h"
#include "processor.h"
#include "globalvar.h"
#include "version.h"
#include "mcpat_core_interface.h"

using namespace std;

void graphite_core();
void print_usage(char * argv0);

//---------------------------------------------------------------------------
// [graphite] Main Loop
//---------------------------------------------------------------------------
int main(int argc,char *argv[])
{
   McPAT::opt_for_clk	=true;

   // Read in Arguments
   if ((argc > 1) && (argv[1] == string("-h") || argv[1] == string("--help")))
	{
		print_usage(argv[0]);
	}

	for (int32_t i = 0; i < argc; i++)
	{
		if (argv[i] == string("-opt_for_clk"))
		{
			i++;
         McPAT::opt_for_clk = (bool)atoi(argv[i]);
		}
	}

	cout<<"McPAT (version "<< VER_MAJOR <<"."<< VER_MINOR
		<< " of " << VER_UPDATE << ") is computing the target processor...\n "<<endl;

	/*// Create ParseXML Object
   McPAT::ParseXML *p1= new McPAT::ParseXML();
   // Fill ParseXML Object from XML File Argument
	p1->parse(fb);*/

   /*// Create Processor Object
	Processor proc0(p1);
   Processor proc1(p1);
   Processor proc2(p1);
   proc0.computeEnergy();
   proc1.computeEnergy();
   proc2.computeEnergy();
   proc0.computeEnergy();
   proc1.computeEnergy();
   proc2.computeEnergy();
   proc0.computeEnergy();
   proc1.computeEnergy();
   proc2.computeEnergy();
   // Print Energy from Processor Object
	proc0.displayEnergy(2, plevel);*/

   /*// Create McPATCore Object
   McPAT::McPATCore mcpatcore0(p1);
   mcpatcore0.computeEnergy();
   mcpatcore0.computeEnergy();
   mcpatcore0.computeEnergy();
   mcpatcore0.displayEnergy(2, plevel);

   // Create McPATCache Object
   McPAT::McPATCache mcpatcache0(p1);
   mcpatcache0.computeEnergy();
   mcpatcache0.computeEnergy();
   mcpatcache0.computeEnergy();
   mcpatcache0.displayEnergy(2, plevel);*/

   /*// Delete ParseXML Object
	delete p1;*/

   // Run a Dummy Graphite Core
   graphite_core();

	return 0;
}

//---------------------------------------------------------------------------
// Dummy Graphite Core
//---------------------------------------------------------------------------
void graphite_core()
{
   // Make McPATCoreInterface
   McPATCoreInterface * m_mcpat_core_interface;
   m_mcpat_core_interface = new McPATCoreInterface();

   // Use McPATCoreInterface
   m_mcpat_core_interface->setEventCountersA();
   m_mcpat_core_interface->computeMcPATCoreEnergy();
   m_mcpat_core_interface->displayMcPATCoreEnergy();

   // Delete McPATCoreInterface
   delete m_mcpat_core_interface;
}

//---------------------------------------------------------------------------
// Help Message
//---------------------------------------------------------------------------
void print_usage(char * argv0)
{
    cerr << "How to use McPAT:" << endl;
    cerr << "  mcpat -infile <input file name>  -print_level < level of details 0~5 >  -opt_for_clk < 0 (optimize for ED^2P only)/1 (optimzed for target clock rate)>"<< endl;
    //cerr << "    Note:default print level is at processor level, please increase it to see the details" << endl;
    exit(1);
}
