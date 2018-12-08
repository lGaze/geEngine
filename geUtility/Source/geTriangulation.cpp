#/*****************************************************************************/
/**
 * @file    geTriangulation.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2017/10/15
 * @brief   Contains helper methods that triangulate point data.
 *
 * Contains helper methods that triangulate point data.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geTriangulation.h"
#include "geVector3.h"
#include "Externals/TetGen/tetgen.h"

namespace geEngineSDK {
  TetrahedronVolume
  Triangulation::tetrahedralize(const Vector<Vector3>& points) {
    TetrahedronVolume volume;
    SIZE_T numPoints = points.size();
    if (numPoints < 4) {  //We need at least 4 points to work with
      return volume;
    }

    //Set the points to send them to TetGen
    tetgenio input;
    input.numberofpoints = static_cast<int>(numPoints);
    
    //Must be allocated with "new" because TetGen deallocates it using "delete"
    input.pointlist = new REAL[input.numberofpoints * 3];
    for (SIZE_T i = 0; i < numPoints; ++i) {
      input.pointlist[i * 3 + 0] = points[i].x;
      input.pointlist[i * 3 + 1] = points[i].y;
      input.pointlist[i * 3 + 2] = points[i].z;
    }

    //Configure the behavior of the library
    tetgenbehavior options;
    options.neighout = 2; //Generate adjacency information between tets and outer faces
    options.facesout = 1; //Output face adjacency
    options.quiet = 1;    //Don't print anything

    //Execute tetrahedralization using the library
    tetgenio output;
    ::tetrahedralize(&options, &input, &output);

    //Copy results to the return object
    auto numTetrahedra = static_cast<SIZE_T>(output.numberoftetrahedra);
    volume.tetrahedra.resize(numTetrahedra);

    for (SIZE_T i = 0; i < numTetrahedra; ++i) {
      memcpy(volume.tetrahedra[i].vertices,
             &output.tetrahedronlist[i * 4],
             sizeof(int32) * 4);

      memcpy(volume.tetrahedra[i].neighbors,
             &output.neighborlist[i * 4],
             sizeof(int32) * 4);
    }

    //Generate boundary faces
    auto numFaces = static_cast<uint32>(output.numberoftrifaces);
    for (uint32 i = 0; i < numFaces; ++i) {
      int32 tetIdx = -1;
      if (-1 == output.adjtetlist[i * 2]) {
        tetIdx = output.adjtetlist[i * 2 + 1];
      }
      else if (-1 == output.adjtetlist[i * 2 + 1]) {
        tetIdx = output.adjtetlist[i * 2];
      }
      else {  //Not a boundary face
        continue;
      }

      volume.outerFaces.emplace_back();
      TetrahedronFace& face = volume.outerFaces.back();

      memcpy(face.vertices, &output.trifacelist[i * 3], sizeof(int32) * 3);
      face.tetrahedron = tetIdx;
    }

    //Ensure that vertex at the specified location points a neighbor opposite to it
    for (uint32 i = 0; i < numTetrahedra; ++i) {
      int32 neighbors[4];
      memcpy(neighbors, volume.tetrahedra[i].neighbors, sizeof(int32) * 4);

      for (uint32 j = 0; j < 4; ++j) {
        int32 vert = volume.tetrahedra[i].vertices[j];

        for (int32 neighborIdx : neighbors) {
          if (neighborIdx == -1) {
            continue;
          }

          Tetrahedron& neighbor = volume.tetrahedra[neighborIdx];
          if (vert != neighbor.vertices[0] &&
              vert != neighbor.vertices[1] &&
              vert != neighbor.vertices[2] &&
              vert != neighbor.vertices[3]) {
            volume.tetrahedra[i].neighbors[j] = neighborIdx;
            break;
          }
        }
      }
    }

    return volume;
  }
}
