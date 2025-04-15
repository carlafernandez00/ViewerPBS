// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <triangle_mesh.h>

#include <algorithm>
#include <limits>

#include <iostream>
#include <glm/geometric.hpp>

namespace data_representation {

TriangleMesh::TriangleMesh() { Clear(); }

void TriangleMesh::Clear() {
  vertices_.clear();
  faces_.clear();
  normals_.clear();
  texCoords_.clear();

  min_ = glm::vec3(std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max());
  max_ = glm::vec3(std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest(),
                         std::numeric_limits<float>::lowest());
}

void TriangleMesh::computeNormals()
{
    normals_.clear();
    std::vector<std::vector<int> > facesPerVertex(vertices_.size()/3);

    //compute normals per face and facesPerVertex structure
    std::vector<float> fNormals(faces_.size());
    for(auto i = 0; i < faces_.size(); i+=3) {
        glm::vec3 v1(vertices_[3*faces_[i]],      vertices_[3*faces_[i]+1],   vertices_[3*faces_[i]+2]);
        glm::vec3 v2(vertices_[3*faces_[i+1]],    vertices_[3*faces_[i+1]+1], vertices_[3*faces_[i+1]+2]);
        glm::vec3 v3(vertices_[3*faces_[i+2]],    vertices_[3*faces_[i+2]+1], vertices_[3*faces_[i+2]+2]);

        glm::vec3 v12 = v2-v1;
        glm::vec3 v13 = v3-v1;
        glm::normalize(v12);
        glm::normalize(v13);

        glm::vec3 n = glm::cross(v12,v13);
        fNormals[i] = n.x;    fNormals[i+1] = n.y;  fNormals[i+2] = n.z;

        facesPerVertex[faces_[i]].push_back(i/3);
        facesPerVertex[faces_[i+1]].push_back(i/3);
        facesPerVertex[faces_[i+2]].push_back(i/3);
    }

    //compute normals per vertex
    normals_.reserve(vertices_.size());
    for(auto i = 0; i < vertices_.size(); ++i) {
        glm::vec3 n;
        for(auto j = 0; j < facesPerVertex[i/3].size(); ++j) {
            n += glm::vec3(fNormals[j*3], fNormals[j*3+1], fNormals[j*3+2]);
        }
        glm::normalize(n);
        normals_[i] = n.x;    normals_[i+1] = n.y;  normals_[i+2] = n.z;
    }
    std::cout << "Normals computed!" << std::endl;
}

}  // namespace data_representation
