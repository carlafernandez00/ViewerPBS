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


const std::vector<std::string> kGBufferShaderFiles = {
    "../shaders/gbuffer.vert", "../shaders/gbuffer.frag"
};
const std::vector<std::string> kFinalShaderFiles = {
    "../shaders/quad.vert", "../shaders/final.frag"
};
const std::vector<std::string> kSSAOShaderFiles = {
      "../shaders/quad.vert", "../shaders/ssao.frag"  // Pure SSAO calculation
  };
const std::vector<std::string> kBlurShaderFiles = {
    "../shaders/quad.vert", "../shaders/blur.frag"  // Blur pass
};

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

float quadVertices[] = {
    // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
    // NOTE that this plane is now much smaller and at the top of the screen
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
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

bool LoadImage(const std::string &path, GLuint cube_map_pos, int mip_level = 0) {
  QImage image;
  bool res = image.load(path.c_str());
  if (res) {
    QImage gl_image = image.mirrored();
    glTexImage2D(cube_map_pos, mip_level, GL_RGBA, image.width(), image.height(), 0,
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
      albedo_(1.0, 1.0, 1.0),
      useTextures_(false),
      applyGammaCorrection_(false),
      // SSAO
      SSAO_enabled_(false),
      currentSSAORenderMode_(0), // 0 - Normals, 1 - Albedo, 2 - Depth, 3 - SSAO, 4 - Blurred SSAO, 5 - Final Composition
      ssao_num_directions_(32),
      ssao_samples_per_direction_(6),
      ssao_sample_radius_(0.5f),
      use_randomization_(false),
      use_blur_(false),
      blur_type_(2),            // Bilateral blur
      blur_radius_(2.0f),
      normal_threshold_(0.8f),
      depth_threshold_(0.01f),
      bias_angle_(0.1f),
      ao_strength_(1.0f)
      {
  setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget() {
  if (initialized_) {
    glDeleteTextures(1, &specular_map_);
    glDeleteTextures(1, &diffuse_map_);
    glDeleteTextures(1, &brdfLUT_map_);
    glDeleteTextures(1, &color_map_);
    glDeleteTextures(1, &roughness_map_);
    glDeleteTextures(1, &metalness_map_);
    glDeleteTextures(1, &weighted_specular_map_);

    glDeleteFramebuffers(1, &g_buffer_FBO_);
    glDeleteTextures(1, &albedo_texture_);
    glDeleteTextures(1, &normal_texture_);
    glDeleteTextures(1, &depth_texture_);
    glDeleteFramebuffers(1, &ssao_FBO_);
    glDeleteTextures(1, &ssao_texture_);
    glDeleteFramebuffers(1, &blur_FBO_);
    glDeleteTextures(1, &blurred_ssao_texture_);
    glDeleteTextures(1, &noise_texture_);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_v);
    glDeleteBuffers(1, &VBO_n);
    glDeleteBuffers(1, &VBO_tc);
    glDeleteBuffers(1, &VBO_i);
     glDeleteVertexArrays(1, &VAO_sky);
    glDeleteBuffers(1, &VBO_v_sky);
    glDeleteBuffers(1, &VBO_i_sky);
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

bool GLWidget::LoadWeightedSpecularMap(const QString &dir) {
  glBindTexture(GL_TEXTURE_CUBE_MAP, weighted_specular_map_);
  bool res = LoadCubeMap(dir);

  // Generate mipmaps for the specular map
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  update();
  return res;
}

bool GLWidget::LoadBRDFLUTMap(const QString &filename)
{
    glBindTexture(GL_TEXTURE_2D, brdfLUT_map_);

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

  // Add this to check OpenGL version
  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version = glGetString(GL_VERSION);
  const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
  
  std::cout << "Renderer: " << renderer << std::endl;
  std::cout << "OpenGL version: " << version << std::endl;
  std::cout << "GLSL version: " << glslVersion << std::endl;
    

  //initializing opengl state
  // glEnable(GL_NORMALIZE); // Deprecated in core profile, causes GL_INVALID_ENUM on macOS
  glDisable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_DEPTH_TEST);

  //generating needed textures
  glGenTextures(1, &specular_map_);
  glGenTextures(1, &diffuse_map_);
  glGenTextures(1, &weighted_specular_map_);
  glGenTextures(1, &brdfLUT_map_);
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

  // Load SSAO shaders
  gbuffer_program_ = std::make_unique<QOpenGLShaderProgram>();
  res = LoadProgram(kGBufferShaderFiles[0], kGBufferShaderFiles[1], gbuffer_program_.get());
  if (!res) {
    std::cerr << "Error loading G-Buffer shader." << std::endl;
    exit(0);
  }

  // Load SSAO shader
  ssao_program_ = std::make_unique<QOpenGLShaderProgram>();
  res = LoadProgram(kSSAOShaderFiles[0], kSSAOShaderFiles[1], ssao_program_.get());
  if (!res) {
      std::cerr << "Error loading SSAO shader." << std::endl;
      exit(0);
  }

  // Load blur shader
  blur_program_ = std::make_unique<QOpenGLShaderProgram>();
  res = LoadProgram(kBlurShaderFiles[0], kBlurShaderFiles[1], blur_program_.get());
  if (!res) {
      std::cerr << "Error loading blur shader." << std::endl;
      exit(0);
  }

  // Load final composition shader
  final_program_ = std::make_unique<QOpenGLShaderProgram>();
  res = LoadProgram(kFinalShaderFiles[0], kFinalShaderFiles[1], final_program_.get());
  if (!res) {
      std::cerr << "Error loading final composition shader." << std::endl;
      exit(0);
  }

  LoadModel(".null"); // Load a sphere as default model
  // LoadModel("../models/mercedes-benz-clk430-convertible_ply/Mercedes-Benz CLK 430 Convertible.ply");

  // Load textures and cube maps
  LoadDefaultMaterials();

  InitializeSSAO();
  initialized_ = true;
}

// Helper to print framebuffer status as string
static const char* fbStatusString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default: return "Unknown";
    }
}

void GLWidget::InitializeSSAO() {
  // Use actual widget size for G-buffer textures
  GLint scrWidth = static_cast<GLint>(width_ > 0 ? width_ : 600);
  GLint scrHeight = static_cast<GLint>(height_ > 0 ? height_ : 600);

  // Clean up previous FBO/textures if they exist
  if (g_buffer_FBO_) glDeleteFramebuffers(1, &g_buffer_FBO_);
  if (albedo_texture_) glDeleteTextures(1, &albedo_texture_);
  if (normal_texture_) glDeleteTextures(1, &normal_texture_);
  if (depth_texture_) glDeleteTextures(1, &depth_texture_);

  // generate textures where info will be stored
  glGenTextures(1, &albedo_texture_);
  glBindTexture(GL_TEXTURE_2D, albedo_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &normal_texture_);
  glBindTexture(GL_TEXTURE_2D, normal_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glGenTextures(1, &depth_texture_);
  glBindTexture(GL_TEXTURE_2D, depth_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, scrWidth, scrHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  GLenum depthError = glGetError();
  if (depthError != GL_NO_ERROR) {
      std::cerr << "OpenGL error after depth texture creation: " << depthError << std::endl;
  }

  // Generate the G-Buffer FBO to attach the textures
  glGenFramebuffers(1, &g_buffer_FBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_FBO_);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_texture_, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_texture_, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture_, 0);

  GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, drawBuffers);

  // Print FBO status and texture IDs for debugging
  GLenum fbStatus2 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  // std::cerr << "[InitializeSSAO] FBO status after glDrawBuffers: 0x" << std::hex << fbStatus2 << " (" << fbStatusString(fbStatus2) << ")" << std::endl;
  // std::cerr << "[InitializeSSAO] albedo_texture_=" << albedo_texture_ << ", normal_texture_=" << normal_texture_ << std::endl;

  GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
      qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status:" << fbStatus << fbStatusString(fbStatus);
      std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! Status: 0x" << std::hex << fbStatus << " (" << fbStatusString(fbStatus) << ")" << std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Generate VAO and VBO to render the quad (only once)
  static bool quad_initialized = false;
  if (!quad_initialized) {
    glGenVertexArrays(1, &quad_VAO);
    glBindVertexArray(quad_VAO);
    glGenBuffers(1, &quad_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    quad_initialized = true;
    }

  // check errors
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
      std::cerr << "OpenGL error at line " << __LINE__ << ": " << error << std::endl;
  }

  // SSAO texture
  glGenTextures(1, &ssao_texture_);
  glBindTexture(GL_TEXTURE_2D, ssao_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Generate the SSAO FBO to attach the texture
  glGenFramebuffers(1, &ssao_FBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_FBO_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_texture_, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Create blurred SSAO texture
  glGenTextures(1, &blurred_ssao_texture_);
  glBindTexture(GL_TEXTURE_2D, blurred_ssao_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Generate the blurred SSAO FBO to attach the texture
  glGenFramebuffers(1, &blur_FBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, blur_FBO_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurred_ssao_texture_, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Create Noise texture for SSAO
  const int noise_size = scrWidth > scrHeight ? scrWidth : scrHeight;
  std::vector<glm::vec3> noise_data(noise_size * noise_size);
  
  // Generate random vectors
  for (int i = 0; i < noise_size * noise_size; i++) {
      glm::vec3 noise_vec(
          (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
          (rand() / float(RAND_MAX)) * 2.0f - 1.0f,
          0.0f  // Keep Z = 0 for rotation around Z-axis
      );
      noise_data[i] = glm::normalize(noise_vec);
  }

  glGenTextures(1, &noise_texture_);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, &noise_data[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

}

void GLWidget::LoadDefaultMaterials(){
  // Initialize a Specular CubeMap
  bool specular_loaded = LoadSpecularMap("../textures/Lycksele2/sky"); 
  if (!specular_loaded) {
    std::cerr << "Error loading specular cube map." << std::endl;
  }
  // Initialize a Diffuse CubeMap
  bool diffuse_loaded = LoadDiffuseMap("../textures/Lycksele2/irradiance_map"); 
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

  // Initialize a BRDF LUT Map
  bool brdf_loaded = LoadBRDFLUTMap("../textures/Lycksele2/brdf_lut.png");
  if (!brdf_loaded) {
    std::cerr << "Error loading brdf LUT map." << std::endl;
  }

  // Initialize a Weighted Specular CubeMap
  bool weighted_specular_loaded = LoadWeightedSpecularMap("../textures/Lycksele2/specular_prefilter");
  if (!weighted_specular_loaded) {
    std::cerr << "Error loading weighted specular cube map." << std::endl;
  }
}

void GLWidget::resizeGL (int w, int h)
{
    if (h == 0) h = 1;
    width_ = w;
    height_ = h;

    camera_.SetViewport(0, 0, w, h);
    camera_.SetProjection(kFieldOfView, kZNear, kZFar);
    InitializeSSAO(); // Ensure G-buffer matches window size
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
      // reset SSAO shaders
      gbuffer_program_.reset();
      gbuffer_program_ = std::make_unique<QOpenGLShaderProgram>();
      LoadProgram(kGBufferShaderFiles[0], kGBufferShaderFiles[1], gbuffer_program_.get());

      ssao_program_.reset();
      ssao_program_ = std::make_unique<QOpenGLShaderProgram>();
      LoadProgram(kSSAOShaderFiles[0], kSSAOShaderFiles[1], ssao_program_.get());

      blur_program_.reset();
      blur_program_ = std::make_unique<QOpenGLShaderProgram>();
      LoadProgram(kBlurShaderFiles[0], kBlurShaderFiles[1], blur_program_.get());

      final_program_.reset();
      final_program_ = std::make_unique<QOpenGLShaderProgram>();
      LoadProgram(kFinalShaderFiles[0], kFinalShaderFiles[1], final_program_.get());
  }

  update();
}

void GLWidget::renderMesh(glm::mat4x4 model, glm::mat4x4 view, glm::mat4x4 projection, glm::mat3 normal)
{
  GLint projection_location, view_location, model_location,
  normal_matrix_location, specular_map_location, diffuse_map_location, brdfLUT_map_location,
  weighted_specular_map_location, fresnel_location, color_map_location, roughness_map_location, metalness_map_location,
  current_text_location, light_location, camera_location, roughness_location, metalness_location, 
  use_textures_location, apply_gamma_correction_location, albedo_location;

  //general shader setting
  programs_[currentShader_]->bind();
  
  projection_location             = programs_[currentShader_]->uniformLocation("projection");
  view_location                   = programs_[currentShader_]->uniformLocation("view");
  model_location                  = programs_[currentShader_]->uniformLocation("model");
  normal_matrix_location          = programs_[currentShader_]->uniformLocation("normal_matrix");
  specular_map_location           = programs_[currentShader_]->uniformLocation("specular_map");
  diffuse_map_location            = programs_[currentShader_]->uniformLocation("diffuse_map");
  weighted_specular_map_location  = programs_[currentShader_]->uniformLocation("weighted_specular_map");
  brdfLUT_map_location            = programs_[currentShader_]->uniformLocation("brdfLUT_map");
  color_map_location              = programs_[currentShader_]->uniformLocation("color_map");
  roughness_map_location          = programs_[currentShader_]->uniformLocation("roughness_map");
  metalness_map_location          = programs_[currentShader_]->uniformLocation("metalness_map");
  current_text_location           = programs_[currentShader_]->uniformLocation("current_texture");
  fresnel_location                = programs_[currentShader_]->uniformLocation("fresnel");
  light_location                  = programs_[currentShader_]->uniformLocation("light");
  camera_location                 = programs_[currentShader_]->uniformLocation("camera_position");
  roughness_location              = programs_[currentShader_]->uniformLocation("roughness");
  metalness_location              = programs_[currentShader_]->uniformLocation("metalness");
  albedo_location                 = programs_[currentShader_]->uniformLocation("albedo");
  use_textures_location           = programs_[currentShader_]->uniformLocation("use_textures");
  apply_gamma_correction_location = programs_[currentShader_]->uniformLocation("apply_gamma_correction");

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

  // Weighted Specular CubeMap
  glActiveTexture(GL_TEXTURE6);
  glBindTexture(GL_TEXTURE_CUBE_MAP, weighted_specular_map_);
  glUniform1i(weighted_specular_map_location, 6);

  // BRDF LUT Texture
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, brdfLUT_map_);
  glUniform1i(brdfLUT_map_location, 2);

  // Textures
  // Color Map (Texture unit 3)
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, color_map_);
  glUniform1i(color_map_location, 3);

  // Roughness Map (Texture unit 4)
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, roughness_map_);
  glUniform1i(roughness_map_location, 4);

  // Metalness Map (Texture unit 5)
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_2D, metalness_map_);
  glUniform1i(metalness_map_location, 5);

  glUniform1i(current_text_location, currentTexture_);
  glUniform3f(fresnel_location, fresnel_[0], fresnel_[1], fresnel_[2]);
  glUniform3f(light_location, 10, 0, 0);
  glUniform3f(camera_location, camera_.GetPosition().x, camera_.GetPosition().y, camera_.GetPosition().z);
  glUniform1f(roughness_location, roughness_);
  glUniform1f(metalness_location, metalness_);
  glUniform3f(albedo_location, albedo_[0], albedo_[1], albedo_[2]);
  glUniform1i(use_textures_location, useTextures_ ? 1 : 0);
  glUniform1i(apply_gamma_correction_location, applyGammaCorrection_ ? 1 : 0);

  // Bind the VAO and draw the elements
  glBindVertexArray(VAO);
  glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
  glBindVertexArray(0);
}


void GLWidget::renderSkybox(glm::mat4x4 model, glm::mat4x4 view, glm::mat4x4 projection, glm::mat3 normal) 
{
  GLint projection_location, view_location, model_location,
  normal_matrix_location, specular_map_location;

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

void GLWidget::renderDefault ()
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
            renderMesh(model, view, projection, normal);

            if(skyVisible_) {
                renderSkybox(model, view, projection, normal);
            }
        }

    }
}

void GLWidget::renderWithSSAO ()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (initialized_) {
      // define default framebuffer object (FBO) for rendering
      GLuint defaultFBO = QOpenGLContext::currentContext()->defaultFramebufferObject();
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

      // Activate Textures
      glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, albedo_texture_);
      glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, normal_texture_);
      glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, depth_texture_);
      glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, noise_texture_);
      glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, ssao_texture_);
      glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, blurred_ssao_texture_);

      // FIRST PASS: G-Buffer generation
      glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_FBO_);
      // Check framebuffer status after binding
      // GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      // if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
      //     std::cerr << "[renderWithSSAO] FBO incomplete! Status: 0x" << std::hex << fbStatus << " (" << fbStatusString(fbStatus) << ")" << std::endl;
      // }

      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (mesh_ != nullptr) {
          GLint projection_location, view_location, model_location, normal_matrix_location, albedo_location;

          gbuffer_program_->bind();

          projection_location     = gbuffer_program_->uniformLocation("projection");
          view_location           = gbuffer_program_->uniformLocation("view");
          model_location          = gbuffer_program_->uniformLocation("model");
          normal_matrix_location  = gbuffer_program_->uniformLocation("normal_matrix");
          albedo_location         = gbuffer_program_->uniformLocation("albedo");

          glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection[0][0]);
          glUniformMatrix4fv(view_location, 1, GL_FALSE, &view[0][0]);
          glUniformMatrix4fv(model_location, 1, GL_FALSE, &model[0][0]);
          glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, &normal[0][0]);
          glUniform3f(albedo_location, albedo_[0], albedo_[1], albedo_[2]);

          glBindVertexArray(VAO);
          glDrawElements(GL_TRIANGLES, mesh_->faces_.size(), GL_UNSIGNED_INT, (GLvoid*)0);
          glBindVertexArray(0);
      }

      // PASS 2: SSAO calculation → Pure AO output
      glBindFramebuffer(GL_FRAMEBUFFER, ssao_FBO_);
      glViewport(0, 0, width_, height_);
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ssao_program_->bind();

      // Textures
      glUniform1i(ssao_program_->uniformLocation("normal_texture"), 1);
      glUniform1i(ssao_program_->uniformLocation("depth_texture"), 2);
      glUniform1i(ssao_program_->uniformLocation("noise_texture"), 3);

      // Set SSAO parameters
      glUniform1i(ssao_program_->uniformLocation("num_directions"), ssao_num_directions_);
      glUniform1i(ssao_program_->uniformLocation("samples_per_direction"), ssao_samples_per_direction_);
      glUniform1f(ssao_program_->uniformLocation("sample_radius"), ssao_sample_radius_);
      glUniformMatrix4fv(ssao_program_->uniformLocation("projection"), 1, GL_FALSE, &projection[0][0]);
      glUniform2f(ssao_program_->uniformLocation("viewport_size"), width_, height_);

      glUniform2f(ssao_program_->uniformLocation("noise_scale"), width_/4.0f, height_/4.0f);
      glUniform1f(ssao_program_->uniformLocation("zNear"), static_cast<float>(kZNear));
      glUniform1f(ssao_program_->uniformLocation("zFar"), static_cast<float>(kZFar));
      glUniform1f(ssao_program_->uniformLocation("fov"), static_cast<float>(kFieldOfView));
      glUniform1i(ssao_program_->uniformLocation("use_randomization"), use_randomization_ ? 1 : 0);
      glUniform1f(ssao_program_->uniformLocation("bias_angle"), bias_angle_);

      glBindVertexArray(quad_VAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
      ssao_program_->release();

      // PASS 3: Blur SSAO texture
      glBindFramebuffer(GL_FRAMEBUFFER, blur_FBO_);
      glViewport(0, 0, width_, height_);
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      blur_program_->bind();

      // Send textures to the shader
      glUniform1i(blur_program_->uniformLocation("ssao_texture"), 4);
      glUniform1i(blur_program_->uniformLocation("normal_texture"), 1);
      glUniform1i(blur_program_->uniformLocation("depth_texture"), 2);

      glUniform2f(blur_program_->uniformLocation("viewport_size"), width_, height_);
      glUniform1i(blur_program_->uniformLocation("blur_type"), blur_type_);
      glUniform1f(blur_program_->uniformLocation("blur_radius"), blur_radius_);
      glUniform1f(blur_program_->uniformLocation("normal_threshold"), normal_threshold_);
      glUniform1f(blur_program_->uniformLocation("depth_threshold"), depth_threshold_);

      glBindVertexArray(quad_VAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
      blur_program_->release();

      // FINAL STEP: Render to the screen
      GLuint albedo_texture_location, normal_texture_location, depth_texture_location, ssao_texture_location, 
      ssao_render_mode_location, z_near_location, z_far_location, use_blur_location, blur_ssao_texture_location;

      glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      final_program_->bind();
      albedo_texture_location         = final_program_->uniformLocation("albedo_texture");
      normal_texture_location         = final_program_->uniformLocation("normal_texture");
      depth_texture_location          = final_program_->uniformLocation("depth_texture");
      ssao_texture_location           = final_program_->uniformLocation("ssao_texture");
      ssao_render_mode_location       = final_program_->uniformLocation("ssao_render_mode");
      z_near_location                 = final_program_->uniformLocation("zNear");
      z_far_location                  = final_program_->uniformLocation("zFar");
      use_blur_location               = final_program_->uniformLocation("use_blurred_ssao");
      blur_ssao_texture_location      = final_program_->uniformLocation("blurred_ssao_texture");

      glUniform1i(albedo_texture_location, 0);      // albedo texture
      glUniform1i(normal_texture_location, 1);      // normal texture
      glUniform1i(depth_texture_location, 2);       // depth texture
      glUniform1i(ssao_texture_location, 4);        // SSAO texture
      glUniform1i(blur_ssao_texture_location, 5);   // Blur SSAO texture
      glUniform1i(ssao_render_mode_location, currentSSAORenderMode_);
      glUniform1i(use_blur_location, use_blur_ ? 1 : 0);

      glUniform1f(z_near_location, static_cast<float>(kZNear));
      glUniform1f(z_far_location, static_cast<float>(kZFar));
      
      glBindVertexArray(quad_VAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
      final_program_->release();
    }
}

void GLWidget::paintGL ()
{
  if (SSAO_enabled_) {
      renderWithSSAO();
  }
  else {
    renderDefault();
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

void GLWidget::SetUseTextures(bool use) {
    useTextures_ = use;
    update();
}

void GLWidget::ApplyGammaCorrection(bool apply) {
    applyGammaCorrection_ = apply;
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

void GLWidget::SetSSAODirections(int directions) {
    ssao_num_directions_ = std::max(4, directions);
    update();
}

void GLWidget::SetSSAOSamplesPerDirection(int samples) {
    ssao_samples_per_direction_ = std::max(1, samples);
    update();
}

void GLWidget::SetSSAORadius(double radius) {
    ssao_sample_radius_ = static_cast<float>(std::max(0.01, radius));
    update();
}

void GLWidget::SetSSAORenderMode(int mode) {
    currentSSAORenderMode_ = mode;
    update();
}

void GLWidget::EnableSSAO(bool enable) {
    SSAO_enabled_ = enable;
    update();
}

void GLWidget::SetUseRandomization(bool use) {
    use_randomization_ = use;
    update();
}

void GLWidget::SetUseBlur(bool use) {
    use_blur_ = use;
    update();
}

void GLWidget::SetBlurType(int type) {
    blur_type_ = std::max(0, std::min(3, type));
    update();
}

void GLWidget::SetBlurRadius(double radius) {
    blur_radius_ = static_cast<float>(std::max(1.0, std::min(10.0, radius)));
    update();
}

void GLWidget::SetNormalThreshold(double threshold) {
    normal_threshold_ = static_cast<float>(std::max(0.0, std::min(1.0, threshold)));
    update();
}

void GLWidget::SetDepthThreshold(double threshold) {
    depth_threshold_ = static_cast<float>(std::max(0.001, std::min(0.1, threshold)));
    update();
}

void GLWidget::SetBiasAngle(double angle) {
    bias_angle_ = static_cast<float>(std::max(0.0, std::min(0.5, angle)));
    update();
}

void GLWidget::SetAOStrength(double strength) {
    ao_strength_ = static_cast<float>(std::max(0.0, std::min(2.0, strength)));
    update();
}

