/// \file
/// Declares animated particle clusters that follow a source object.
#ifndef PARTICLE_CLUSTER_H
#define PARTICLE_CLUSTER_H

#include <ultra64.h>

/// Create an animated particle cluster at a source object.
/// \param count Positive number of particles; drawing and matrix allocation sizes must fit in 32 bits.
/// \param lifetime Cluster lifetime in seconds.
/// \param radialScale Scale controlling the particles' radial extent.
/// \param particleScale Scale controlling the particles' thickness.
/// \param source Live source object supplying the cluster's position and target handle.
/// \returns Registered cluster object, managed by the object system.
/// \note Original decompilation identifier: \c func_80019B08 (0x80019B08).
u8 *particleClusterCreate(s32 count, f32 lifetime, f32 radialScale, f32 particleScale, u8 *source);

#endif
