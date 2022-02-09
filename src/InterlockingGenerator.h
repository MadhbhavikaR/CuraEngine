//Copyright (c) 2018 Ultimaker B.V.
//CuraEngine is released under the terms of the AGPLv3 or higher.

#ifndef INTERLOCKING_GENERATOR_H
#define INTERLOCKING_GENERATOR_H

#include <vector>
#include <cassert>
#include <unordered_set>

#include "utils/polygon.h"
#include "utils/VoxelUtils.h"

namespace cura
{

class Slicer;

/*!
 * Class for generating an interlocking structure between two adjacent models of a different extruder.
 * 
 * The structure consists of horizontal beams of the two materials interlaced.
 * In the z direction the direction of these beams is alternated with 90*.
 * 
 * Example with two materials # and O
 * Even beams:      Odd beams:
 * ######           ##OO##OO
 * OOOOOO           ##OO##OO
 * ######           ##OO##OO
 * OOOOOO           ##OO##OO
 * 
 * One mateiral of a single cell of the structure looks like this:
 *                    .-*-.
 *                .-*       *-.
 *               |*-.           *-.
 *               |    *-.           *-.
 *            .-* *-.     *-.           *-.
 *        .-*         *-.     *-.       .-*|
 *    .-*           .-*   *-.     *-.-*    |
 *   |*-.       .-*     .-*   *-.   |   .-*
 *   |    *-.-*     .-*           *-|-*
 *    *-.   |   .-*
 *        *-|-*
 * 
 * We set up a voxel grid of (2*beam_w,2*beam_w,2*beam_h) and mark all the voxels which contain both meshes.
 * We then remove all voxels which also contain air, so that the interlocking pattern will not be visible from the outside.
 * We then generate and combine the polygons for each voxel and apply those areas to the outlines ofthe meshes.
 */
class InterlockingGenerator
{
public:
    /*!
     * Generate an interlocking structure between each two adjacent meshes.
     */
    static void generateInterlockingStructure(std::vector<Slicer*>& volumes);

protected:
    /*!
     * Generate an interlocking structure between two meshes
     */
    static void generateInterlockingStructure(Slicer& mesh_a, Slicer& mesh_b);

    /*!
     * Private class for storing some variables used in the computation of the interlocking structure between two meshes.
     * \param mesh_a The first mesh
     * \param mesh_b The second mesh
     * \param line_width_per_mesh The line widths of both meshes
     * \param max_layer_count The maximum of the two layer counts
     * \param rotation The angle by which to rotate the interlocking pattern
     * \param cell_size The size of a voxel cell in (coord_t, coord_t, layer_count)
     * \param beam_layer_count The number of layers for the height of the beams
     * \param air_filtering Whether to fully remove all of the interlocking cells which would be visible on the outside. If no air filtering then those cells will be cut off midway in a beam.
     */
    InterlockingGenerator(Slicer& mesh_a, Slicer& mesh_b, coord_t (& line_width_per_mesh)[2], const size_t max_layer_count, const PointMatrix& rotation, Point3 cell_size, coord_t beam_layer_count, bool air_filtering)
    : mesh_a(mesh_a)
    , mesh_b(mesh_b)
    , line_width_per_mesh(line_width_per_mesh)
    , max_layer_count(max_layer_count)
    , vu(cell_size)
    , rotation(rotation)
    , cell_size(cell_size)
    , beam_layer_count(beam_layer_count)
    , air_filtering(air_filtering)
    {}

    /*!
     * Compute the voxels overlapping with the shell of both models.
     * This includes the walls, but also top/bottom skin.
     * 
     * \param kernel The dilation kernel to give the returned voxel shell more thickness
     * \return The shell voxels for mesh a and those for mesh b
     */
    std::vector<std::unordered_set<GridPoint3>> getShellVoxels(const DilationKernel& kernel) const;
    
    /*!
     * Compute the voxels overlapping with the shell of some layers.
     * This includes the walls, but also top/bottom skin.
     * 
     * \param layers The layer outlines for which to compute the shell voxels
     * \param kernel The dilation kernel to give the returned voxel shell more thickness
     * \param[out] cells The output cells which elong to the shell
     */
    void addBoundaryCells(std::vector<Polygons>& layers, const DilationKernel& kernel, std::unordered_set<GridPoint3>& cells) const;

    /*!
     * Compute the regions occupied by both models.
     * 
     * A morphological close is performed so that we don't register small gaps between the two models as being separate.
     * \param[out] layer_regions The computed layer regions
     */
    void computeLayerRegions(std::vector<Polygons>& layer_regions) const;

    /*!
     * Generate the polygons for the beams of a single cell
     * \param[out] cell_area_per_mesh_per_layer The output polygons for each beam
     */
    void generateMicrostructure(std::vector<std::vector<Polygons>>& cell_area_per_mesh_per_layer) const;

    /*!
     * Change the outlines of the meshes with the computed interlocking structure.
     * 
     * \param cells The cells where we want to apply the interlocking structure.
     * \param cell_area_per_mesh_per_layer The layer polygons of a single cell
     * \param layer_regions The total volume of the two meshes combined (and small gaps closed)
     */
    void applyMicrostructureToOutlines(const std::unordered_set<GridPoint3>& cells, std::vector<std::vector<Polygons>>& cell_area_per_mesh_per_layer, const std::vector<Polygons>& layer_regions) const;

    static const coord_t ignored_gap = 100u; //!< Distance between models to be considered next to each other so that an interlocking structure will be generated there

    Slicer& mesh_a;
    Slicer& mesh_b;
    coord_t (& line_width_per_mesh)[2]; // reference to an array of length 2
    const size_t max_layer_count;

    VoxelUtils vu;

    PointMatrix rotation;
    Point3 cell_size;
    coord_t beam_layer_count;
    bool air_filtering; //!< Whether to fully remove all of the interlocking cells which would be visible on the outside. If no air filtering then those cells will be cut off midway in a beam.
};

}//namespace cura

#endif//INTERLOCKING_GENERATOR_H
