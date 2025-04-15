// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#ifndef MESH_IO_H_
#define MESH_IO_H_

#include <triangle_mesh.h>

#include <string>

namespace data_representation {

static bool hasNormals = false;

/**
 * @brief ReadFromPly Read the mesh stored in PLY format at the path filename
 * and stores the corresponding TriangleMesh representation
 * @param filename The path to the PLY mesh.
 * @param mesh The resulting representation with computed per-vertex normals.
 * @return Whether it was able to read the file.
 */
bool ReadFromPly(const std::string &filename, TriangleMesh *mesh);

/**
 * @brief WriteToPly Stores the mesh representation in PLY format at the path
 * filename.
 * @param filename The path where the mesh will be stored.
 * @param mesh The mesh to be stored.
 * @return Whether it was able to store the file.
 */
bool WriteToPly(const std::string &filename, const TriangleMesh &mesh);

/**
 * @brief ReadFromObj Read the mesh stored in OBJ format at the path filename
 * and stores the corresponding TriangleMesh representation. It only works for
 * models with a unique material.
 * @param filename The path to the OBJ mesh.
 * @param mesh The resulting representation with computed per-vertex normals.
 * @return Whether it was able to read the file.
 */
bool ReadFromObj(const std::string &filename, TriangleMesh *mesh);

/**
 * @brief CreateSphere Creates an sphere
 * @param mesh The resulting representation with computed per-vertex normals.
 * @return true
 */
bool CreateSphere(TriangleMesh *mesh);

}  // namespace data_representation

#endif  // MESH_IO_H_
