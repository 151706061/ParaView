/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Pipe set subset.

#ifndef NVIDIA_INDEX_IPIPE_SET_SUBSET_H
#define NVIDIA_INDEX_IPIPE_SET_SUBSET_H

#include <nv/index/idistributed_data_subset.h>

namespace nv {
namespace index {

class IPipe_set_subset;
class IPipe_set_subset_device;


/// Helper for setting the parameters of a single pipe inside a subset. An instance of this
/// interface is returned by \c IPipe_set_subset::add_pipe().
///
/// \ingroup nv_index_data_subsets
///
class IPipe_subset_parameters :
    public mi::base::Interface_declare<0x3ee3429b,0x0607,0x4b87,0xb3,0xe6,0x9e,0x94,0x57,0x60,0x54,0x41>
{
public:
    /// Sets the radius of the pipe.
    ///
    /// \param[in] radius      Radius to be used for all vertices of the pipe.
    ///
    /// \return true on success, or false if the radius is 0 or negative.
    //
    virtual bool set_radius(mi::Float32 radius) = 0;

    /// Sets the color of a pipe. This is the base color only, which may be influenced by the
    /// assigned material during rendering, depending on the active rendering kernel program.
    ///
    /// \param[in] color       Base color to be used for all vertices of the pipe.
    ///
    virtual void set_color(const mi::math::Color_struct& color) = 0;

    /// Sets custom user flags of a pipe. The flags are passed through to rendering kernel programs
    /// and are not interpreted in any way by NVIDIA IndeX.
    ///
    /// \param[in] flags       Custom user flags.
    ///
    virtual void set_user_flags(mi::Uint32 flags) = 0;

    /// Sets the property that should be mapped onto the pipe. The property must already have been
    /// reserverved by \c reserve_property().
    ///
    /// \param[in] property_name  Unique name identifying the property, must already be reserved.
    ///
    /// \return true on success, or false if \c property_name is invalid or the property was not yet
    ///          reserved.
    //
    virtual bool set_property(const char* property_name) = 0;

    /// Sets the per-vertex coordinates for mapping property data onto the pipe.
    ///
    /// \param[in] property_coordinates     Array of 1D coordinates, or 0.
    /// \param[in] nb_property_coordinates  Number of coordinates in the array, must either be 0 or
    ///                                     match the number of vertices given to \c add_pipe().
    ///
    /// \return true on success, or false if one of the arguments is invalid.
    //
    virtual bool set_property_coordinates(
        const mi::Float32* property_coordinates,
        mi::Size           nb_property_coordinates) = 0;

    /// Specifies the domain-specific property coordinate values of the first and last vertex of the
    /// pipe. If this is not set, or when \c coordinate_first <= \c coordinate_last, then the entire
    /// coordinate range will be used.
    ///
    /// This should be used together with the corresponding parameters of \c
    /// IPipe_set_subset::set_property_data().
    ///
    /// \param[in] coordinate_first         Domain-specific property coordinate value of the first vertex.
    /// \param[in] coordinate_last          Domain-specific property coordinate value of the last vertex.
    ///
    virtual void set_property_coordinates_range(
        mi::Float32 coordinate_first,
        mi::Float32 coordinate_last) = 0;

    /// Sets the expected value range of the property data for the pipe. The data values will be
    /// normalized (but not clipped/clamped) to [0 1], with \c value_min mapped to 0 and \c
    /// value_max mapped to 1.
    ///
    /// If \c value_min or \c value_max is set to +/-infinity then the actual min/max value found in
    /// the data will be used.
    ///
    /// \param[in] value_min  Data value that should be mapped to 0.
    /// \param[in] value_max  Data value that should be mapped to 1.
    ///
    virtual void set_property_value_range(
        mi::Float32 value_min,
        mi::Float32 value_max) = 0;

    /// Sets information about the given pipe for the current subset/subregion. Since it is not
    /// possible to determine from the vertex data alone whether the overall start and end point of
    /// the entire subregion is contained in the current subset, providing this information allows
    /// for better optimization. However, it is also safe to always set \c true (which is the
    /// default).
    ///
    /// \param[in] contains_start_point  Pipe start point is contained in this subregion.
    /// \param[in] contains_end_point    Pipe end point is contained in this subregion.
    ///
    virtual void set_subset_status(
        mi::Float32 contains_start_point,
        mi::Float32 contains_end_point) = 0;
};

/// Distributed data storage for pipe sets.
///
/// \ingroup nv_index_data_subsets
///
class IPipe_set_subset :
    public mi::base::Interface_declare<0xea834969,0x1c60,0x4416,0xbd,0x05,0x3a,0x4b,0xbe,0x78,0xef,0x5f,
                                       IDistributed_data_subset>
{
public:
    /// Returns the bounding box of the subset.
    ///
    /// \return Bounding box in the local coordinate system.
    ///
    virtual const mi::math::Bbox_struct<mi::Float32, 3>& get_bounding_box() const = 0;

    /// Adds a new pipe with the given vertices.
    ///
    /// \param[in] vertices     Array of vertices
    /// \param[in] nb_vertices  Number of vertices in the array
    /// \return The new pipe that was just added to the subset. The caller takes ownership, but
    ///         should not store it longer than necessary. It must be released before \c finalize() is
    ///         called.
    ///
    virtual IPipe_subset_parameters* add_pipe(
        const mi::math::Vector_struct<mi::Float32, 3>* vertices,
        mi::Size                                       nb_vertices) = 0;

    /// Returns whether data is available for the given property, or if it at least has been
    /// reserved.
    ///
    /// \param[in] property_name  Unique name identifying the property.
    /// \return true if property data is available or has been reserved.
    ///
    virtual bool has_property(const char* property_name) const = 0;

    /// Tries to reserve the given property to the caller, so that it can start loading the property
    /// data. This approach is necessary to ensure property data won't get loaded by multiple
    /// concurrent threads.
    ///
    /// \param[in] property_name  Unique name identifying the property.
    /// \return true if reservation succeeded, or false if the property was already reserved.
    ///
    virtual bool reserve_property(const char* property_name) const = 0;

    /// Stores the data for the given property. To prevent interference between multiple threads,
    /// this should only get called by the thread for which \c reserve_property() returned true.
    ///
    /// Optionally, the property coordinates for the first and last property value may be specified.
    /// This should be used together with \c IPipe_subset_parameters::set_property_coordinates_range().
    ///
    /// \param[in] property_name       Unique name identifying the property.
    /// \param[in] property_values     Array of property values.
    /// \param[in] nb_property_values  Number of values in the array.
    /// \param[in] coordinate_first    Coordinate of the first property value.
    /// \param[in] coordinate_last     Coordinate of the last property value.
    /// \return true on success, or false if one of the arguments is invalid.
    ///
    virtual bool set_property_data(
        const char*        property_name,
        const mi::Float32* property_values,
        mi::Size           nb_property_values,
        mi::Float32        coordinate_first = 0.f,
        mi::Float32        coordinate_last = 0.f) = 0;

    /// Access the device pipe set subset for direct access to device resources.
    ///
    /// \return     Device subset interface for direct access of device resources.
    ///             May return \c NULL if data is not on a GPU device.
    /// 
    virtual IPipe_set_subset_device* get_device_subset() const = 0;

};


/// Distributed data storage class for pipe set subsets hosted on a GPU device.
///
/// Pipe_set data on GPU device can be completely redefined through this interface. 
/// Note that data changes through this interface might invalidate pipe set data of the associated host \c IPipe_set_subset.
///
/// \ingroup nv_index_data_subsets
///
class IPipe_set_subset_device :
    public mi::base::Interface_declare<0x5b91ea8e,0xda21,0x4134,0x87,0x4,0x6b,0xf2,0xd3,0xae,0x66,0xf7,
                                       IDistributed_data_subset_device>
{
public:
    /// List of supported color data formats.
    enum Color_format 
    {
        Color_rgb_f32,  ///< 3 floats RGB
        Color_rgba_f32  ///< 4 floats RGBA
    };

    /// Load a complete pipe set. This redefines all pipe set data. Property data will be invalidated.
    ///
    /// \param[in] data_is_on_device    Set true if the provided data is already on device. 
    /// \param[in] vertices             Array of vertices (all pipes of pipe set), size: nb_vertices.
    /// \param[in] nb_vertices          Number of vertices.
    /// \param[in] nb_pipes             Number of pipes in pipe set.
    /// \param[in] pipe_indices         Array of pipe vertex indices, size: (nb_pipes + 1), contents: [0, ..., nb_vertices].
    /// \param[in] default_radius       Default pipe radius, used if no \c radii array is provided.
    /// \param[in] default_color        Default pipe color, used of no \c colors array is provided.
    /// \param[in] radii                Array of pipe radii, size: nb_pipes. Optional, can be nullptr.
    /// \param[in] colors               Array of pipe colors, size: nb_pipes. Optional, can be nullptr.
    /// \param[in] color_format         Color data format in \c colors array, \see Color_format.
    /// \return true on succes.
    ///
    virtual bool    load_pipe_set(
        bool                                            data_is_on_device,
        const mi::math::Vector_struct<mi::Float32, 3>*  vertices,
        mi::Uint32                                      nb_vertices,
        mi::Uint32                                      nb_pipes,
        const mi::Uint32*                               pipe_indices,           // size: nb_pipes + 1
        mi::Float32                                     default_radius,         // used if no per-pipe radii
        const mi::math::Color_struct&                   default_color,          // used if no per-pipe colors
        const mi::Float32*                              radii,                  // size: nb_pipes
        const void*                                     colors,                 // size: nb_pipes * sizeof(Color_format)
        Color_format                                    color_format) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPIPE_SET_SUBSET_H
