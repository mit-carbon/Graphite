#ifndef ADDRESSHOMELOOKUP_H_
#define ADDRESSHOMELOOKUP_H_

#include "assert.h"
#include "pin.H"
#include "debug.h"
#include <iostream> 
#include <sstream> 
#include <vector> 
#include <math.h>

using namespace std;

/* TODO abstract MMU stuff to a configure file to allow
 * user to specify number of memory controllers, and
 * the address space that each is in charge of.  Default behavior:
 * AHL assumes that every core has a memory controller,
 * and that each shares an equal number of address spaces.
 *
 * Now possible to setAddrBoundaries, either through constructor
 * or during runtime!
 *
 * each core has an AHL, but they must keep their data consistent 
 * regarding boundaries!
 *
 * Maybe allow the ability to have public and private memory space?
 */

class AddressHomeLookup
{
	public:
		AddressHomeLookup(UINT32 num_nodes);
		AddressHomeLookup(vector< pair<ADDRINT,ADDRINT> > addr_bounds);
		virtual ~AddressHomeLookup();
	  
		/* TODO: change this return type to a node number type */
		UINT32 find_home_for_addr(ADDRINT) const;
		void setAddrBoundaries(vector< pair<ADDRINT, ADDRINT> > addr_bounds);

	private:
		//TODO make these static since all instances need to be consistent?
		vector< pair<ADDRINT, ADDRINT> > address_boundaries;
		vector<UINT64> bytes_per_core;

	   UINT32 num_nodes;
};

#endif /*ADDRESSHOMELOOKUP_H_*/
