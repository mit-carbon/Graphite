#ifndef DVFS_H
#define DVFS_H

#ifdef __cplusplus
extern "C"
{
#endif

void CarbonGetTileFrequency(volatile float* frequency);
void CarbonSetTileFrequency(volatile float* frequency);

#ifdef __cplusplus
}
#endif

#endif // DVFS_H
