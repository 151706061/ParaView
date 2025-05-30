/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces for data samples returned for example by \c IScene_pick_results instances.

#ifndef NVIDIA_INDEX_IDATA_SAMPLE_H
#define NVIDIA_INDEX_IDATA_SAMPLE_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv {
namespace index {

/// Base-interface class for all data-sample interface classes. A data sample is a single
/// value generated by, for example, a regular volume lookup. It may represent a filtered value
/// from multiple neighboring voxels in a regular volume. Data samples are returned, for instance,
/// as part of a scene picking result (c.f. \c IScene_pick_result) when intersecting either a
/// regular volume dataset or a geometry textured with such a volume dataset.
///
/// \ingroup nv_scene_queries
///
class IData_sample :
    public mi::base::Interface_declare<0xb0dccbeb,0x889,0x4f82,0x85,0xf,0x1,0x62,0xaa,0xae,0xe,0x76>
{
};

/// Intermediate-interface class for all typed data-sample interfaces. According to the actual
/// dataset component type (e.g. Uint8, Uint16, Float32, Rgba8) the data sample value can be retrieved.
///
/// \ingroup nv_scene_queries
///
template<typename T>
class IData_sample_typed :
    public mi::base::Interface_declare<0x610e173e,0x591b,0x4d65,0xa7,0x9e,0xf6,0xc3,0x33,0x50,0x8f,0x80,
                                       IData_sample>
{
public:
    typedef T Value_type;   ///<! Type of the sampled data.

public:
    /// Returns the regular volume sample value according to the specific volume voxel type.
    ///
    /// \returns    Regular volume sample value.
    ///
    virtual T get_sample_value() const = 0;
};

/// Single-channel 8bit unsigned data sample.
///
/// \ingroup nv_scene_queries
///
class IData_sample_uint8 :
    public mi::base::Interface_declare<0xe274699f,0xb1a6,0x400d,0xb5,0xea,0xdf,0x50,0xbd,0x3e,0x34,0x8b,
                                       IData_sample_typed<mi::Uint8> >
{
};

/// Single-channel 16bit unsigned data sample.
///
/// \ingroup nv_scene_queries
///
class IData_sample_uint16 :
    public mi::base::Interface_declare<0x9b8cc7d3,0xf850,0x4354,0xac,0xa4,0x20,0xd7,0x26,0xb1,0xd6,0x29,
                                       IData_sample_typed<mi::Uint16> >
{
};

/// Single-channel 32bit floating point data sample.
///
/// \ingroup nv_scene_queries
///
class IData_sample_float32 :
    public mi::base::Interface_declare<0x70c53d3a,0x91b2,0x4ef2,0xb9,0x60,0x4b,0x45,0x68,0xe1,0x8,0x24,
                                       IData_sample_typed<mi::Float32> >
{
};

/// Four-channel 8bit per channel unsigned integer data sample.
///
/// \ingroup nv_scene_queries
///
class IData_sample_rgba8 :
    public mi::base::Interface_declare<0x7e71a021,0x1e26,0x4ad8,0xb0,0x1,0x68,0x6d,0x97,0xf9,0x63,0xde,
                                       IData_sample_typed<mi::math::Vector_struct<mi::Uint8, 4> > >        
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDATA_SAMPLE_H
