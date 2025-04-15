// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#ifndef TRIANGLE_MESH_H_
#define TRIANGLE_MESH_H_

#include <glm/vec3.hpp>

#include <vector>
#include <string>

namespace data_representation {

class TriangleMesh {
 public:
  /**
   * @brief TriangleMesh Constructor of the class. Calls clear.
   */
  TriangleMesh();

  /**
   * @brief ~TriangleMesh Destructor of the class.
   */
  ~TriangleMesh() {}

  /**
   * @brief Clear Empties the data arrays and resets the bounding box vertices.
   */
  void Clear();

  /**
  * @brief Compute vertex normals of the model
  */
  void computeNormals();

 public:
  std::vector<float> vertices_;
  std::vector<int> faces_;
  std::vector<float> normals_;
  std::vector<float> texCoords_;
  std::string diffuseMap_;

  /**
   * @brief min The minimum point of the bounding box.
   */
  glm::vec3 min_;

  /**
   * @brief max The maximum point of the bounding box.
   */
  glm::vec3 max_;
};

}  // namespace data_representation

#endif  //  TRIANGLE_MESH_H_
