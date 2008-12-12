#ifndef NETWORK_MESH_ANALYTICAL_PARAMS_H
#define NETWORK_MESH_ANALYTICAL_PARAMS_H

// These are retrieved from global config object
struct NetworkMeshAnalyticalParameters
{
  double Tw2;         // wire delay between adjacent nodes on mesh
  double s;           // switch delay, relative to Tw2
  int n;              // dimension of network
  double W2;          // channel width constraint (constraint
  // is on bisection width, W2 * N held constant) (paper
  // normalized to unit-width channels, W2 denormalizes)
  double p;           // network utilization
};

#endif // NETWORK_MESH_ANALYTICAL_PARAMS_H
