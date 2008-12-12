#ifndef NETWORK_MESH_ANALYTICAL_PARAMS_H
#define NETWORK_MESH_ANALYTICAL_PARAMS_H

// These are retrieved from global config object
struct NetworkMeshAnalyticalParameters
{
  double Tw2;         // wire delay between adjacent nodes on mesh
  double s;           // switch delay, relative to Tw2
  int n;              // dimension of network
  double W;           // channel width
};

#endif // NETWORK_MESH_ANALYTICAL_PARAMS_H
