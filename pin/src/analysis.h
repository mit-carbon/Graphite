// Analysis
//
// This file is responsible for the functions needed during analysis time
// to determine the data that needs to be passed on to the simulator
// during run time.
#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <set>
#include "pin.H"
#include "perfmdl.h"
#include "fixed_types.h"
#include "bit_vector.h"
#include "run_models.h"

void getPotentialLoadFirstUses(const RTN& rtn, set<INS>& ins_uses);
bool insertInstructionModelingCall(const string& rtn_name, const INS& start_ins,
                                   const INS& ins, bool is_rtn_ins_head, bool is_bbl_ins_head,
                                   bool is_bbl_ins_tail, bool is_potential_load_use);

#endif
