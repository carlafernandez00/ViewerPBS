// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <glwidget.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#include "./mesh_io.h"
#include "./triangle_mesh.h"

#include <glm/mat4x4.hpp>

namespace {

const double kFieldOfView = 60;
const double kZNear = 0.0001;
const double kZFar = 20;

const std::vector<std::vector<std::string>> kShaderFiles = {
                {"../shaders/phong.vert",        "../shaders/phong.frag"},
                {"../shaders/texMap.vert",       "../shaders/texMap.frag"},
                {"../shaders/reflection.vert",   "../shaders/reflection.frag"},
                {"../shaders/pbs.vert",          "../shaders/pbs.frag"},
                {"../shaders/ibl-pbs.vert",      "../shaders/ibl-pbs.frag"},
                {"../shaders/sky.vert",          "../shaders/sky.frag"}};//sky needs to be the last one



const int kVertexAttributeIdx = 0;
const int kNormalAttributeIdx = 1;
const int kTexCoordAttributeIdx = 2;

// Define the Skybox
const float kSkySize = 10.0f;
float kSkyVertices[24] = {
    -0.5f * kSkySize, -0.5f * kSkySize, -0.5f * kSkySize,	
     0.5f * kSkySize, -0.5f * kSkySize, -0.5f * kSkySize,	
    -0.5f * kSkySize, -0.5f * kSkySize,  0.5f * kSkySize, 
     0.5f * kSkySize, -0.5f * kSkySize,  0.5f * kSkySize,
    -0.5f * kSkySize,  0.5f * kSkySize, -0.5f * kSkySize,
     0.5f * kSkySize,  0.5f * kSkySize, -0.5f * kSkySize,
    -0.5f * kSkySize,  0.5f * kSkySize,  0.5f * kSkySize, 
     0.5f * kSkySize,  0.5f * kSkySize,  0.5f * kSkySize
};

unsigned int kSkyFaces[36] = {
  // Top
  4, 7, 6,
  4, 5, 7,
  // Bottom
  0, 3, 1,
  0, 2, 3,
  // Back
  6, 3, 2,
  6, 7, 3,
  // Front
  0, 1, 4,
  4, 1, 5,
  // Left
  6, 0, 2,
  6, 4, 0,
  // Right
  1, 3, 7,
  7, 5, 1
};

bool ReadFile(const std::string filename, std::string *shader_source) {
  std::ifstream infile(filename.c_str());

  if (!infile.is_open() || !infile.good()) {
    std::cerr << "Error " + filename + " not found." << std::endl;
    return false;
  }

  std::stringstream stream;
  stream << infile.rdbuf();
  infile.close();

  *shader_source = stream.str();
  return true;
}

bool LoadImage(const std::string &path, GLuint cube_map_pos) {
  QImage image;
  bool res = image.load(path.c_str());
  if (res) {
    QImage gl_image = image.mirrored();
    glTexImage2D(cube_map_pos, 0, GL_RGBA, image.width(), image.height(), 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
  }
  return res;
}

bool LoadCubeMap(const QString &dir) {
  std::string path = dir.toUtf8().constData();
  bool res = LoadImage(path + "/right.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
  res = res && LoadImage(path + "/left.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
  res = res && LoadImage(path + "/top.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
  res = res && LoadImage(path + "/bottom.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
  res = res && LoadImage(path + "/back.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
  res = res && LoadImage(path + "/front.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

  if (res) {
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }

  return res;
}

bool LoadProgram(const std::string &vertex, const std::string &fragment,
                 QOpenGLShaderProgram *program) {
  std::string vertex_shader, fragment_shader;
  bool res =
      ReadFile(vertex, &vertex_shader) && ReadFile(fragment, &fragment_shader);

  if (res) {
    program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                     vertex_shader.c_str());
    program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                     fragment_shader.c_str());
    program->bindAttributeLocation("vertex", kVertexAttributeIdx);
    program->bindAttributeLocation("normal", kNormalAttributeIdx);
    program->bindAttributeLocation("texCoord", kTexCoordAttributeIdx);
    program->link();
  }

  return res;
}

}  // namespace

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      initialized_(false),
      width_(0.0),
      height_(0.0),
      currentShader_(0),
      currentTexture_(0),
      fresnel_(0.2, 0.2, 0.2),
      skyVisible_(true),
      metalness_(0),
      roughness_(0),
      albedo_(1.0, 1.0, 1.0)
      {
  setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget() {
  if (initialized_) {
    glDeleteTextures(1, &specular_map_);
    glDeleteTextures(1, &diffuse_map_);
  }
}

bool GLWidget::LoadModel(const QString &filename) {
  std::string file = filename.toUtf8().constData();
  size_t pos = file.find_last_of(".");
  std::string type = file.substr(pos + 1);

  std::unique_ptr<data_representation::TriangleMesh> mesh =
      std::make_unique<data_representation::TriangleMesh>();

  bool res = false;
  if (type.compare("ply") == 0) {
    res = data_representation::ReadFromPly(file, mesh.get());
  } else if (type.compare("obj") == 0) {
    res = data_representation::ReadFromObj(file, mesh.get());
  } else if(type.compare("null") == 0) {
    res = data_representation::CreateSphere(mesh.get());
  }

  if (res) {
    mesh_.reset(mesh.release());
    camera_.UpdateModel(mesh_->min_, mesh_->max_);
    // mesh_->computeNormals();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error at line " << __LINE__ << ": " << error << std::endl;
    }

    // Generate VAO
    glGenVertexArrays(1, &VAO); 

    // Create vertices VBOs
    glGenBuffers(1, &VBO_v);
    glGenBuffers(1, &VBO_n);
    glGenBuffers(1, &VBO_tc);
    glGenBuffers(1, &VBO_i);

    // bind VAO 
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_v);
    glBufferData(GL_ARRAY_BUFFER, mesh_->vertices_.size() * sizeof(float), &mesh_->vertices_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(kVertexAttributeIdx, 3, GL_FLOAT, GL_FALSE, 0, (void *)0); // mesh_->vertices -> attrib location 0
    glEnableVertexAttribArray(kVertexAttributeIdx); 

    glBindBuffer(GL_ARRAY_BUFFER, VBO_n);
    glBufferData(GL_ARRAY_BUFFER, mesh_->normals_.size() * sizeof(float), &mesh_->normals_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(kNormalAttributeIdx, 3, GL_FLOAT, GL_FALSE, 0, (void *)0); // mesh_->normals -> attrib location 1
    glEnableVertexAttribArray(kNormalAttributeIdx);

  
    glBindBuffer(GL_ARRAY_BUFFER, VBO_tc);
    glBufferData(GL_ARRAY_BUFFER, mesh_->texCoords_.size() * sizeof(float), &mesh_->texCoords_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(kTexCoordAttributeIdx, 2, GL_FLOAT, GL_FALSE, 0, (void *)0); // mesh_->texCoords -> attrib location 2
    glEnableVertexAttribArray(kTexCoordAttributeIdx);

    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_->faces_.size() * sizeof(int), &mesh_->faces_[0], GL_STATIC_DRAW);

    // unbind VAO
    glBindVertexArray(0);

    //SKY BOX:
    // Store the vertices and faces of the skybox
    skyVertices_.assign(kSkyVertices, kSkyVertices + sizeof(kSkyVertices)/sizeof(float));
    skyFaces_.assign(kSkyFaces, kSkyFaces + sizeof(kSkyFaces)/sizeof(unsigned int));

    // Generate the VAO and VBOs for the skybox
    glGenVertexArrays(1, &VAO_sky);
    glGenBuffers(1, &VBO_v_sky);
    glGenBuffers(1, &VBO_i_sky);

    // bind VAO
    glBindVertexArray(VAO_sky);

    // vertices -> attrib location 0
    glBindBuffer(GL_ARRAY_BUFFER, VBO_v_sky);
    glBufferData(GL_ARRAY_BUFFER, skyVertices_.size() * sizeof(float), &skyVertices_[0], GL_STATIC_DRAW);
    glVertexAttribPointer(kVertexAttributeIdx, 3, GL_FLOAT, GL_FALSE, 0, (void *)0); 
    glEnableVertexAttribArray(kVertexAttributeIdx);

    // faces -> elements
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO_i_sky);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, skyFaces_.size() * sizeof(int), &skyFaces_[0], GL_STATIC_DRAW);
    
    // unbind VAO
    glBindVertexArray(0);

    // unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);



    emit SetFaces(QString(std::to_string(mesh_->faces_.size() / 3).c_str()));
    emit SetVertices(
        QString(std::to_string(mesh_->vertices_.size() / 3).c_str()));
    return true;
  }

  return false;
}

bool GLWidget::LoadSpecularMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
  bool res = LoadCubeMap(dir);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  update();
  return res;
}

bool GLWidget::LoadDiffuseMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
  bool res = LoadCubeMap(dir);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  update();
  return res;
}

bool GLWidget::LoadColorMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, color_map_);

    std::string path = filename.toUtf8().constData();
    bool res = LoadImage(path, GL_TEXTURE_2D);

    // Set texture parameters
    if (res) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps for better quality at different distances
    }

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    update();
    return res;

}

bool GLWidget::LoadRoughnessMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, roughness_map_);
    
    std::string path = filename.toUtf8().constData();
    bool res = LoadImage(path, GL_TEXTURE_2D);

    // Set texture parameters
    if (res) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps for better quality at different distances
    }

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
    
    update();
    return res;
}

bool GLWidget::LoadMetalnessMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, metalness_map_);

    std::string path = filename.toUtf8().constData();
    bool res = LoadImage(path, GL_TEXTURE_2D);
    
    // Set texture parameters
    if (res) {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

      glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps for better quality at different distances
    }

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    update();
    return res;
}

void GLWidget::initializeGL ()
{
  // Cal inicialitzar l'ús de les funcions d'OpenGL
  initializeOpenGLFunctions();

  //initializing opengl state
  glEnable(GL_NORMALIZE);
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);

  //generating needed textures
  glGenTextures(1, &specular_map_);
  glGenTextures(1, &diffuse_map_);
  glGenTextures(1, &color_map_);
  glGenTextures(1, &roughness_map_);
  glGenTextures(1, &metalness_map_);

  //create shader programs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//phong
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//texture mapping
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//reflection
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//simple pbs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//ibl pbs
  programs_.push_back(std::make_unique<QOpenGLShaderProgram>());//sky

  //load vertex and fragment shader files
  bool res =   LoadProgram(kShaderFiles[0][0],   kShaderFiles[0][1],    programs_[0].get());
  res = res && LoadProgram(kShaderFiles[1][0],   kShaderFiles[1][1],    programs_[1].get());
  res = res && LoadProgram(kShaderFiles[2][0],   kShaderFiles[2][1],    programs_[2].get());
  res = res && LoadProgram(kShaderFiles[3][0],   kShaderFiles[3][1],    programs_[3].get());
  res = res && LoadProgram(kShaderFiles[4][0],   kShaderFiles[4][1],    programs_[4].get());
  res = res && LoadProgram(kShaderFiles[5][0],   kShaderFiles[5][1],    programs_[5].get());

  if (!res) exit(0);

  LoadModel(".null");//create an sphere

  initialized_ = true;

  // Initialize a Specular CubeMap
  bool specular_loaded = LoadSpecularMap("../textures/desert_specular"); 
  if (!specular_loaded) {
    std::cerr << "Error loading specular cube map." << std::endl;
  }
  // Initialize a Diffuse CubeMap
  bool diffuse_loaded = LoadDiffuseMap("../textures/desert_specular"); 
  if (!diffuse_loaded) {
    std::cerr << "Error loading diffuse cube map." << std::endl;
  }

  // Initialize a Color Map
  bool color_loaded = LoadColorMap("../textures/Metal053C_2K-PNG_Color.png");
  if (!color_loaded) {
    std::cerr << "Error loading color map." << std::endl;
  }

  // Initialize a Roughness Map
  bool roughness_loaded = LoadRoughnessMap("../textures/Metal053C_2K-PNG_Roughness.png");
  if (!roughness_loaded) {
    std::cerr << "Error loading roughness map." << std::endl;
  }

  // Initialize a Metalness Map
  bool metalness_loaded = LoadMetalnessMap("../textures/Metal053C_2K-PNG_Metalness.png");
  if (!metalness_loaded) {
    std::cerr << "Error loading metalness map." << std::endl;
  }


}

void GLWidget::resizeGL (int w, int h)
{
    if (h == 0) h = 1;
    width_ = w;
    height_ = h;

    camera_.SetViewport(0, 0, w, h);
    camera_.SetProjection(kFieldOfView, kZNear, kZFar);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StartRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StartZooming(event->x(), event->y());
  }
  update();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
  camera_.SetRotationX(event->y());
  camera_.SetRotationY(event->x());
  camera_.SafeZoom(event->y());
  update();
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    camera_.StopRotating(event->x(), event->y());
  }
  if (event->button() == Qt::RightButton) {
    camera_.StopZooming(event->x(), event->y());
  }
  update();
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Up) camera_.Zoom(-1);
  if (event->key() == Qt::Key_Down) camera_.Zoom(1);

  if (event->key() == Qt::Key_Left) camera_.Rotate(-1);
  if (event->key() == Qt::Key_Right) camera_.Rotate(1);

  if (event->key() == Qt::Key_W) camera_.Zoom(-1);
  if (event->key() == Qt::Key_S) camera_.Zoom(1);

  if (event->key() == Qt::Key_A) camera_.Rotate(-1);
  if (event->key() == Qt::Key_D) camera_.Rotate(1);

  if (event->key() == Qt::Key_R) {
      for(auto i = 0; i < programs_.size(); ++i) {
          programs_[i].reset();
          programs_[i] = std::make_unique<QOpenGLShaderProgram>();
          LoadProgram(kShaderFiles[i][0], kShaderFiles[i][1], programs_[i].get());
      }
  }

  update();
}


void GLWidget::paintGL ()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (initialized_) {
        camera_.SetViewport();

        glm::mat4x4 projection = camera_.SetProjection();
        glm::mat4x4 view = camera_.SetView();
        glm::mat4x4 model = camera_.SetModel();

        //compute normal matrix
        glm::mat4x4 t = view * model;
        glm::mat3x3 normal;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                normal[i][j] = t[i][j];
         normal = glm::transpose(glm::inverse(normal));

        if (mesh_ != nullptr) {
            GLint projection_location, view_location, model_location,
            normal_matrix_location, specular_map_location, diffuse_map_location,
            fresnel_location, color_map_location, roughness_map_location, metalness_map_location,
            current_text_location, light_location, camera_location, roughness_location, metalness_location, albedo_location;

            //MESH-----------------------------------------------------------------------------------------
            //general shader setting
            programs_[currentShader_]->bind();
            
            projection_location       = programs_[currentShader_]->uniformLocation("projection");
            view_location             = programs_[currentShader_]->uniformLocation("view");
            model_location            = programs_[currentShader_]->uniformLocation("model");
            normal_matrix_location    = programs_[currentShader_]->uniformLocation("normal_matrix");
            specular_map_location     = programs_[currentShader_]->uniformLocation("specular_map");
            diffuse_map_location      = programs_[currentShader_]->uniformLocation("diffuse_map");
            color_map_location        = programs_[currentShader_]->uniformLocation("color_map");
            roughness_map_location    = programs_[currentShader_]->uniformLocation("roughness_map");
            metalness_map_location    = programs_[currentShader_]->uniformLocation("metalness_map");
            current_text_location     = programs_[currentShader_]->uniformLocation("current_texture");
            fresnel_location          = programs_[currentShader_]->uniformLocation("fresnel");
            light_location            = programs_[currentShader_]->uniformLocation("light");
            camera_location           = programs_[currentShader_]->uniformLocation("camera_position");
            roughness_location        = programs_[currentShader_]->uniformLocation("roughness");
            metalness_location        = programs_[currentShader_]->uniformLocation("metalness");
            albedo_location           = programs_[currentShader_]->uniformLocation("albedo");

            // Model, View, Projection and Normal matrices
            glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
            glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
            glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);

            // Specular CubeMap
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
            glUniform1i(specular_map_location, 0);

            // Diffuse CubeMap
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, diffuse_map_);
            glUniform1i(diffuse_map_location, 1);

            // Textures
            // Color Map (Texture unit 3)
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, color_map_);
            glUniform1i(color_map_location, 2);

            // Roughness Map (Texture unit 4)
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, roughness_map_);
            glUniform1i(roughness_map_location, 3);

            // Metalness Map (Texture unit 5)
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, metalness_map_);
            glUniform1i(metalness_map_location, 4);

            glUniform1i(current_text_location, currentTexture_);
            glUniform3f(fresnel_location, fresnel_[0], fresnel_[1], fresnel_[2]);
            glUniform3f(light_location, 10, 0, 0);
            glUniform3f(camera_location, camera_.GetPosition().x, camera_.GetPosition().y, camera_.GetPosition().z);
            glUniform1f(roughness_location, roughness_);
            glUniform1f(metalness_location, metalness_);
            glUniform3f(albedo_location, albedo_[0], albedo_[1], albedo_[2]);

            // Bind the VAO and draw the elements
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
            glBindVertexArray(0);


            //SKY-----------------------------------------------------------------------------------------
            if(skyVisible_) {
                //model = camera_.SetIdentity();

                // Set depth function for skybox
                GLint oldDepthFunc;
                glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
                
                glDepthFunc(GL_LEQUAL);

                programs_[programs_.size()-1]->bind();

                projection_location     = programs_[programs_.size()-1]->uniformLocation("projection");
                view_location           = programs_[programs_.size()-1]->uniformLocation("view");
                model_location          = programs_[programs_.size()-1]->uniformLocation("model");
                normal_matrix_location  = programs_[programs_.size()-1]->uniformLocation("normal_matrix");
                specular_map_location   = programs_[programs_.size()-1]->uniformLocation("specular_map");
    

                glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
                glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
                glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
                glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, specular_map_);
                programs_[programs_.size() - 1]->setUniformValue("specular_map", 0);

                // Bind the VAO and draw the elements
                glBindVertexArray(VAO_sky);
                glDrawElements(GL_TRIANGLES, skyFaces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
                glBindVertexArray(0);
                programs_[programs_.size() - 1]->release();

                // Restore original depth function
                glDepthFunc(oldDepthFunc);
            }
        }
    }
}

void GLWidget::SetReflection(bool set) {
    if(set) currentShader_ = 2;
    update();
}

void GLWidget::SetPBS(bool set) {
    if(set) currentShader_ = 3;
    update();
}

void GLWidget::SetIBLPBS(bool set) {
    if(set) currentShader_ = 4;
    update();
}

void GLWidget::SetPhong(bool set)
{
    if(set) currentShader_ = 0;
    update();
}

void GLWidget::SetTexMap(bool set)
{
    if(set) currentShader_ = 1;
    update();
}

void GLWidget::SetFresnelR(double r) {
    fresnel_[0] = r;
    update();
}

void GLWidget::SetFresnelG(double g) {
    fresnel_[1] = g;
    update();
}

void GLWidget::SetAlbedo(double r, double g, double b) {
    albedo_[0] = r;
    albedo_[1] = g;
    albedo_[2] = b;
    update();
}
void GLWidget::SetCurrentTexture(int i)
{
    currentTexture_ = i;
    update();
}

void GLWidget::SetSkyVisible(bool set)
{
    skyVisible_ = set;
    update();
}

void GLWidget::SetFresnelB(double b) {
    fresnel_[2] = b;
    update();
}

void GLWidget::SetMetalness(double d) {
    metalness_ = d;
    update();
}

void GLWidget::SetRoughness(double d) {
    roughness_ = d;
    update();
}


