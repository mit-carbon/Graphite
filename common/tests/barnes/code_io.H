#ifndef _CODE_IO_H_
#define _CODE_IO_H_

void inputdata(void);
void initoutput(void);
void output(long ProcessId);
void diagnostics(long ProcessId);
void in_int(stream str, long *iptr);
void in_real(stream str, real *rptr);
void in_vector(stream str, vector vec);
void out_int(stream str, long ival);
void out_real(stream str, real rval);
void out_vector(stream str, vector vec);

#endif
