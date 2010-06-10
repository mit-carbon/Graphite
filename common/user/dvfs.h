#ifndef DVFS_H
#define DVFS_H

#ifdef __cplusplus
extern "C"
{
#endif

void getCoreFrequency(volatile float* frequency);
void setCoreFrequency(volatile float* frequency);

#ifdef __cplusplus
}
#endif

#endif // DVFS_H
