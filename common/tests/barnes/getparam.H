#ifndef _GETPARAM_H_
#define _GETPARAM_H_

void initparam(string *defv);
string getparam(string name);
long getiparam(string name);
long getlparam(string name);
bool getbparam(string name);
double getdparam(string name);
long scanbind(string bvec[], string name);
bool matchname(string bind, string name);
string extrvalue(string arg);

#endif
