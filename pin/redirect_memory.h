#ifndef __REDIRECT_MEMORY_H__
#define __REDIRECT_MEMORY_H__

#include "pin.H"
#include "core.h"
#include "pin_memory_manager.h"

bool rewriteStackOp(INS ins);
void rewriteMemOp(INS ins);

ADDRINT emuPushValue(THREADID thread_id, ADDRINT tgt_esp, ADDRINT value, ADDRINT write_size);
ADDRINT emuPushMem(THREADID thread_id, ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size);
ADDRINT emuPopReg(THREADID thread_id, ADDRINT tgt_esp, ADDRINT *reg, ADDRINT read_size);
ADDRINT emuPopMem(THREADID thread_id, ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size);
ADDRINT emuCallMem(THREADID thread_id, ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT operand_ea, ADDRINT read_size, ADDRINT write_size);
ADDRINT emuCallRegOrImm(THREADID thread_id, ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT br_tgt_ip, ADDRINT write_size);
ADDRINT emuRet(THREADID thread_id, ADDRINT *tgt_esp, UINT32 imm, ADDRINT read_size, BOOL push_info);
ADDRINT emuLeave(THREADID thread_id, ADDRINT tgt_esp, ADDRINT *tgt_ebp, ADDRINT read_size);
ADDRINT redirectPushf(THREADID thread_id, ADDRINT tgt_esp, ADDRINT size);
ADDRINT completePushf(THREADID thread_id, ADDRINT esp, ADDRINT size);
ADDRINT redirectPopf(THREADID thread_id, ADDRINT tgt_esp, ADDRINT size);
ADDRINT completePopf(THREADID thread_id, ADDRINT esp, ADDRINT size);

ADDRINT redirectMemOp(THREADID thread_id, bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, UInt32 op_num, bool is_read);
ADDRINT redirectMemOpSaveEa(ADDRINT ea);
VOID completeMemWrite(THREADID thread_id, bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, UInt32 op_num);

void memOp(THREADID thread_id, Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type, IntPtr d_addr, char *data_buffer, UInt32 data_size, BOOL push_info = true);

#endif /* __REDIRECT_MEMORY_H__ */
