#ifndef __REDIRECT_MEMORY_H__
#define __REDIRECT_MEMORY_H__

#include "pin.H"
#include "core.h"
#include "pin_memory_manager.h"

bool rewriteStringOp (INS ins);
bool rewriteStackOp (INS ins);
void rewriteMemOp (INS ins);

VOID emuCMPSBIns (CONTEXT *ctxt, ADDRINT next_gip, bool has_rep_prefix); 
VOID emuSCASBIns (CONTEXT *ctxt, ADDRINT next_gip, bool has_rep_prefix); 

ADDRINT emuPushValue (ADDRINT tgt_esp, ADDRINT value, ADDRINT write_size);
ADDRINT emuPushMem(ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size);
ADDRINT emuPopReg(ADDRINT tgt_esp, ADDRINT *reg, ADDRINT read_size);
ADDRINT emuPopMem(ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size);
ADDRINT emuCallMem(ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT operand_ea, ADDRINT read_size, ADDRINT write_size);
ADDRINT emuCallRegOrImm(ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT br_tgt_ip, ADDRINT write_size);
ADDRINT emuRet(ADDRINT *tgt_esp, UINT32 imm, ADDRINT read_size, BOOL modeled);
ADDRINT emuLeave(ADDRINT tgt_esp, ADDRINT *tgt_ebp, ADDRINT read_size);
ADDRINT redirectPushf ( ADDRINT tgt_esp, ADDRINT size );
ADDRINT completePushf (ADDRINT esp, ADDRINT size);
ADDRINT redirectPopf (ADDRINT tgt_esp, ADDRINT size);
ADDRINT completePopf (ADDRINT esp, ADDRINT size);

ADDRINT redirectMemOp (bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, PinMemoryManager::AccessType access_type);
VOID completeMemWrite (bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, PinMemoryManager::AccessType access_type);

void memOp (Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type, IntPtr d_addr, char *data_buffer, UInt32 data_size);

#endif /* __REDIRECT_MEMORY_H__ */
