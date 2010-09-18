#ifndef DVFS_H
#define DVFS_H

#ifdef __cplusplus
extern "C"
{
#endif

void CarbonGetCoreFrequency(volatile float* frequency);
void CarbonSetCoreFrequency(volatile float* frequency);

#ifdef __cplusplus
}
#endif

#endif // DVFS_H
