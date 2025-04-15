// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <mesh_io.h>

#include <assert.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include <math.h>

#include "./triangle_mesh.h"
#include "./tiny_obj_loader.h"

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#define TINYOBJLOADER_IMPLEMENTATION

namespace data_representation {

namespace {

template <typename T>
void Add3Items(T i1, T i2, T i3, size_t index, std::vector<T> *vector) {
  (*vector)[index] = i1;
  (*vector)[index + 1] = i2;
  (*vector)[index + 2] = i3;
}

bool ReadPlyHeader(std::ifstream *fin, int *vertices, int *faces) {
  char line[100];

  fin->getline(line, 100);
  if (strncmp(line, "ply", 3) != 0) return false;

  *vertices = 0;
  fin->getline(line, 100);
  while (strncmp(line, "end_header", 10) != 0) {
    if (strncmp(line, "element vertex", 14) == 0) *vertices = atoi(&line[15]);
    fin->getline(line, 100);
    if (strncmp(line, "element face", 12) == 0) *faces = atoi(&line[13]);
    if (strncmp(line, "property float n", 16) == 0) hasNormals = true;
  }

  if (*vertices <= 0) return false;

  std::cout << "Loading triangle mesh" << std::endl;
  std::cout << "\tVertices = " << *vertices << std::endl;
  std::cout << "\tFaces = " << *faces << std::endl;

  return true;
}

void ReadPlyVertices(std::ifstream *fin, TriangleMesh *mesh) {
  const size_t kVertices = mesh->vertices_.size() / 3;
  for (size_t i = 0; i < kVertices; ++i) {
    float x, y, z;
    fin->read(reinterpret_cast<char *>(&x), sizeof(float));
    fin->read(reinterpret_cast<char *>(&y), sizeof(float));
    fin->read(reinterpret_cast<char *>(&z), sizeof(float));

    Add3Items(x, y, z, i * 3, &(mesh->vertices_));
    if(hasNormals) {
        fin->read(reinterpret_cast<char *>(&x), sizeof(float));
        fin->read(reinterpret_cast<char *>(&y), sizeof(float));
        fin->read(reinterpret_cast<char *>(&z), sizeof(float));

        Add3Items(x, y, z, i * 3, &(mesh->normals_));
    }
  }
}

void ReadPlyFaces(std::ifstream *fin, TriangleMesh *mesh) {
  unsigned char vertex_per_face;

  const size_t kFaces = mesh->faces_.size() / 3;
  for (size_t i = 0; i < kFaces; ++i) {
    int v1, v2, v3;
    fin->read(reinterpret_cast<char *>(&vertex_per_face),
              sizeof(unsigned char));
    assert(vertex_per_face == 3);

    fin->read(reinterpret_cast<char *>(&v1), sizeof(int));
    fin->read(reinterpret_cast<char *>(&v2), sizeof(int));
    fin->read(reinterpret_cast<char *>(&v3), sizeof(int));
    Add3Items(v1, v2, v3, i * 3, &(mesh->faces_));
  }
}

void ComputeVertexNormals(const std::vector<float> &vertices,
                          const std::vector<int> &faces,
                          std::vector<float> *normals) {
  const size_t kFaces = faces.size();
  std::vector<float> face_normals(kFaces, 0);

  for (size_t i = 0; i < kFaces; i += 3) {
    glm::vec3 v1(vertices[faces[i] * 3], vertices[faces[i] * 3 + 1],
                       vertices[faces[i] * 3 + 2]);
    glm::vec3 v2(vertices[faces[i + 1] * 3],
                       vertices[faces[i + 1] * 3 + 1],
                       vertices[faces[i + 1] * 3 + 2]);
    glm::vec3 v3(vertices[faces[i + 2] * 3],
                       vertices[faces[i + 2] * 3 + 1],
                       vertices[faces[i + 2] * 3 + 2]);
    glm::vec3 v1v2 = v2 - v1;
    glm::vec3 v1v3 = v3 - v1;
    glm::vec3 normal = glm::cross(v1v2,v1v3);

    if (glm::length(normal) < 0.00001) {
      normal = glm::vec3(0.0, 0.0, 0.0);
    } else {
      glm::normalize(normal);
    }

    for (size_t j = 0; j < 3; ++j) face_normals[i + j] = normal[j];
  }

  const size_t kVertices = vertices.size();
  normals->resize(kVertices, 0);
  for (size_t i = 0; i < kFaces; i += 3) {
    for (size_t j = 0; j < 3; ++j) {
      size_t idx = static_cast<size_t>(faces[i + j]);
      glm::vec3 v1(vertices[faces[i + j] * 3],
                         vertices[faces[i + j] * 3 + 1],
                         vertices[faces[i + j] * 3 + 2]);
      glm::vec3 v2(vertices[faces[i + (j + 1) % 3] * 3],
                         vertices[faces[i + (j + 1) % 3] * 3 + 1],
                         vertices[faces[i + (j + 1) % 3] * 3 + 2]);
      glm::vec3 v3(vertices[faces[i + (j + 2) % 3] * 3],
                         vertices[faces[i + (j + 2) % 3] * 3 + 1],
                         vertices[faces[i + (j + 2) % 3] * 3 + 2]);

      glm::vec3 v1v2 = v2 - v1;
      glm::vec3 v1v3 = v3 - v1;
      double angle = acos(glm::dot(v1v2,v1v3) / (glm::length(v1v2) * glm::length(v1v3)));

      if (angle == angle) {
        for (size_t k = 0; k < 3; ++k) {
          (*normals)[idx * 3 + k] += face_normals[i + k] * angle;
        }
      }
    }
  }

  const size_t kNormals = normals->size();
  for (size_t i = 0; i < kNormals; i += 3) {
    glm::vec3 normal((*normals)[i], (*normals)[i + 1], (*normals)[i + 2]);
    if (glm::length(normal) > 0) {
      glm::normalize(normal);
    } else {
      normal = glm::vec3(0, 0, 0);
    }

    for (size_t j = 0; j < 3; ++j) (*normals)[i + j] = normal[j];
  }
}

void ComputeTexCoords(const std::vector<float> &vertices,
                      std::vector<float> *texCoords) {

    const size_t kVertices = vertices.size()/3;
    texCoords->resize(kVertices*2, 0);
    for(auto i = 0; i < vertices.size()/3; ++i) {

       float longitudeRadians = atan2(vertices[i*3+1], vertices[i*3+0]);
       float latitudeRadians = asin(vertices[i*3+2]);

       // Convert range -PI...PI to 0...1
       float s =  longitudeRadians/(2 * M_PI) + 0.5;

       // Convert range -PI/2...PI/2 to 0...1
       float t =  latitudeRadians/M_PI + 0.5;
       (*texCoords)[i*2] = s;
       (*texCoords)[i*2+1] = t;
    }

}
void ComputeBoundingBox(const std::vector<float> vertices, TriangleMesh *mesh) {
  const size_t kVertices = vertices.size() / 3;
  for (size_t i = 0; i < kVertices; ++i) {
    mesh->min_[0] = std::min(mesh->min_[0], vertices[i * 3]);
    mesh->min_[1] = std::min(mesh->min_[1], vertices[i * 3 + 1]);
    mesh->min_[2] = std::min(mesh->min_[2], vertices[i * 3 + 2]);

    mesh->max_[0] = std::max(mesh->max_[0], vertices[i * 3]);
    mesh->max_[1] = std::max(mesh->max_[1], vertices[i * 3 + 1]);
    mesh->max_[2] = std::max(mesh->max_[2], vertices[i * 3 + 2]);
  }
}

}  // namespace

bool ReadFromPly(const std::string &filename, TriangleMesh *mesh) {
  std::ifstream fin;

  fin.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!fin.is_open() || !fin.good()) return false;

  int vertices = 0, faces = 0;
  if (!ReadPlyHeader(&fin, &vertices, &faces)) {
    fin.close();
    return false;
  }

  mesh->vertices_.resize(static_cast<size_t>(vertices) * 3);
  if(hasNormals) mesh->normals_.resize(static_cast<size_t>(vertices) * 3);
  ReadPlyVertices(&fin, mesh);

  mesh->faces_.resize(static_cast<size_t>(faces) * 3);
  ReadPlyFaces(&fin, mesh);

  fin.close();

  if(!hasNormals) ComputeVertexNormals(mesh->vertices_, mesh->faces_, &mesh->normals_);
  ComputeTexCoords(mesh->vertices_, &mesh->texCoords_);
  ComputeBoundingBox(mesh->vertices_, mesh);

  return true;
}

bool WriteToPly(const std::string &filename, const TriangleMesh &mesh) {
  (void)filename;
  (void)mesh;

  std::cerr << "Not yet implemented" << std::endl;

  // TODO(students): Implement storing to PLY format.

  // END.

  return false;
}

bool ReadFromObj(const std::string &filename, TriangleMesh *mesh)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    std::string baseDir = filename.substr(0, filename.rfind("/"));

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(),baseDir.c_str());

    if (!warn.empty()) {
      std::cout << warn << std::endl;
    }

    if (!err.empty()) {
      std::cerr << err << std::endl;
    }

    if (!ret) {
      exit(1);
    }

    int currentIndex = 0;
    for(const auto& shape: shapes)
    {
        mesh->faces_.resize(mesh->faces_.size()+shape.mesh.indices.size());
        mesh->vertices_.resize((mesh->vertices_.size()+shape.mesh.indices.size())*3);
        if(attrib.normals.size() > 0)
            mesh->normals_.resize((mesh->normals_.size()+shape.mesh.indices.size())*3);
        if(attrib.texcoords.size() > 0)
            mesh->texCoords_.resize((mesh->texCoords_.size()+shape.mesh.indices.size())*2);

        for(const auto& nfv : shape.mesh.num_face_vertices)
            if(size_t(nfv) != 3) {
                std::cerr << "Only supports triangles." << std::endl;
                exit(1);
            }

        for(const auto& index : shape.mesh.indices)
        {
            //auto currentIndex = mesh->faces_.size();
            mesh->faces_[currentIndex] = currentIndex;

            mesh->vertices_[3*currentIndex]   = attrib.vertices[3*index.vertex_index];
            mesh->vertices_[3*currentIndex+1] = attrib.vertices[3*index.vertex_index+1];
            mesh->vertices_[3*currentIndex+2] = attrib.vertices[3*index.vertex_index+2];

            if(attrib.normals.size() > 0) {
                mesh->normals_[3*currentIndex]   = attrib.normals[3*index.normal_index];
                mesh->normals_[3*currentIndex+1] = attrib.normals[3*index.normal_index+1];
                mesh->normals_[3*currentIndex+2] = attrib.normals[3*index.normal_index+2];
            }

            if(attrib.texcoords.size() > 0) {
                mesh->texCoords_[2*currentIndex]   = attrib.texcoords[2*index.texcoord_index];
                mesh->texCoords_[2*currentIndex+1] = 1.f -attrib.texcoords[2*index.texcoord_index+1];
            }

            //std::cout << mesh->vertices_[3*currentIndex] << " " << mesh->vertices_[3*currentIndex+1] << " " << mesh->vertices_[3*currentIndex+2] << std::endl;

            currentIndex++;
        }
    }

    if(attrib.normals.size() == 0)
        ComputeVertexNormals(mesh->vertices_, mesh->faces_, &mesh->normals_);

    ComputeBoundingBox(mesh->vertices_, mesh);

    if(materials.size() > 0)
    {
        mesh->diffuseMap_ = baseDir+"/"+materials[0].diffuse_texname;
    }
    //for(auto i = 0; i < mesh->texCoords_.size(); i+=2)
    //    std::cout << mesh->texCoords_[i] << " " << mesh->texCoords_[i+1] << std::endl;

}

bool CreateSphere(TriangleMesh *mesh)
{
    // clear memory of prev arrays
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
    float radius = 1;
    float sectorCount = 64;
    float stackCount =  64;
    float PI = 3.141592;

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i)
    {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);

            // vertex tex coord (s, t) range between [0, 1]
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            texCoords.push_back(s);
            texCoords.push_back(t);
        }
    }

    std::vector<int> indices;
    int k1, k2;
    for(int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if(i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            // k1+1 => k2 => k2+1
            if(i != (stackCount-1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    mesh->faces_ = indices;
    mesh->vertices_ = vertices;
    mesh->normals_ = normals;
    mesh->texCoords_ = texCoords;

    ComputeBoundingBox(mesh->vertices_, mesh);

    return true;

}

}  // namespace data_representation
