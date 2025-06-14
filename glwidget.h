// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#ifndef GLWIDGET_H_
#define GLWIDGET_H_

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QImage>
#include <QMouseEvent>
#include <QString>

#include <memory>

#include "./camera.h"
#include "./triangle_mesh.h"

#include <glm/vec3.hpp>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
  Q_OBJECT

 public:
  explicit GLWidget(QWidget *parent = nullptr);
  ~GLWidget();

  /**
   * @brief LoadModel Loads a PLY model at the filename path into the mesh_ data
   * structure.
   * @param filename Path to the PLY model.
   * @return Whether it was able to load the model.
   */
  bool LoadModel(const QString &filename);

  /**
   * @brief LoadSpecularMap Will load load a cube map that will be used for the
   * specular component.
   * @param filename Path to the directory containing the 6 textures (right,
   * left, top, bottom, front back) of the sube map that will be used for the
   * specular component.
   * @return Whether it was able to load the textures.
   */
  bool LoadSpecularMap(const QString &filename);

  /**
   * @brief LoadWeightedSpecularMap Will load load a cube map that will be used for the
   * specular component.
   * @param filename Path to the directory containing the 6 textures (right,
   * left, top, bottom, front back) of the sube map that will be used for the
   * specular component.
   * @return Whether it was able to load the textures.
   */
  bool LoadWeightedSpecularMap(const QString &filename);

  /**
   * @brief LoadDiffuseMap Will load load a cube map that will be used for the
   * diffuse component.
   * @param filename Path to the directory containing the 6 textures (right,
   * left, top, bottom, front back) of the sube map that will be used for the
   * diffuse component.
   * @return Whether it was able to load the textures.
   */
  bool LoadDiffuseMap(const QString &filename);

  /**
   * @brief LoadBRDFLUTMap Will load load a texture map that will be used for the
   * specular component.
   * @param filename Path to the texture file.
   * @return Whether it was able to load the texture.
   */
  bool LoadBRDFLUTMap(const QString &filename);

  /**
   * @brief LoadColorMap Will load load a texture map that will be used for the
   * color component.
   * @param filename Path to the texture file.
   * @return Whether it was able to load the texture.
   */
  bool LoadColorMap(const QString &filename);

  /**
   * @brief LoadRoughnessMap Will load load a texture map that will be used for the
   * color component.
   * @param filename Path to the texture file.
   * @return Whether it was able to load the texture.
   */
  bool LoadRoughnessMap(const QString &filename);

  /**
   * @brief LoadMetalnessMap Will load load a texture map that will be used for the
   * color component.
   * @param filename Path to the texture file.
   * @return Whether it was able to load the texture.
   */
  bool LoadMetalnessMap(const QString &filename);

  /**
   * @brief SetAlbedo Sets the albedo color.
   */
  void SetAlbedo(double, double, double);

 protected:
  /**
   * @brief initializeGL Initializes OpenGL variables and loads, compiles and
   * links shaders.
   */
  void initializeGL();

  /**
   * @brief LoadDefaultMaterials Loads the default materials and textures.
   */
  void LoadDefaultMaterials();

  /**
   * @brief InitializeSSAO Initializes the Screen Space Ambient Occlusion (SSAO) effect.
   */
  void InitializeSSAO();

  /**
   * @brief resizeGL Resizes the viewport.
   * @param w New viewport width.
   * @param h New viewport height.
   */
  void resizeGL(int w, int h);

  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void keyPressEvent(QKeyEvent *event);

 private:
  /**
   * @brief programs_ Vector that stores all the needed programs //phong, texMap, reflections, simplePBS, PBS, sky
   */
  std::vector<std::unique_ptr<QOpenGLShaderProgram>> programs_;

  /**
   * @brief SSAO shader programs
   * These programs are used for the G-Buffer pass, SSAO calculation, blur pass, and final composition.
   */
  std::unique_ptr<QOpenGLShaderProgram> gbuffer_program_;   // G-Buffer pass for SSAO
  std::unique_ptr<QOpenGLShaderProgram> ssao_program_;      // Pure SSAO calculation
  std::unique_ptr<QOpenGLShaderProgram> blur_program_;      // Blur pass
  std::unique_ptr<QOpenGLShaderProgram> final_program_;     // Final composition


  /**
   * @brief camera_ Class that computes the multiple camera transform matrices.
   */
  data_visualization::Camera camera_;

  /**
   * @brief mesh_ Data structure representing a triangle mesh.
   */
  std::unique_ptr<data_representation::TriangleMesh> mesh_;

  /**
   * @brief diffuse_map_ Diffuse cubemap texture.
   */
  GLuint diffuse_map_;

  /**
   * @brief specular_map_ Diffuse cubemap texture.
   */
  GLuint specular_map_;

  /**
   * @brief weighted_specular_map_ Weighted specular cubemap texture.
   */
  GLuint weighted_specular_map_;

  /**
   * @brief brdfLUT_map_  BRDF LUT texture.
   */
  GLuint brdfLUT_map_;

  /**
   * @brief color_map_ Color texture.
   */
  GLuint color_map_;

  /**
   * @brief roughness_map_ Roughness texture.
   */
  GLuint roughness_map_;

  /**
   * @brief metalness_map_ Metalness texture.
   */
  GLuint metalness_map_;

  /**
   * @brief albedo_texture_ Albedo texture.
   */
  GLuint albedo_texture_;

  /**
   * @brief normal_texture_ Normal texture.
   * This texture stores the normals of the scene for SSAO.
   */
  GLuint normal_texture_;

  /**
   * @brief depth_texture_ Depth texture.
   * This texture stores the depth information of the scene for SSAO.
   */
  GLuint depth_texture_;

  /**
   * @brief g_buffer_FBO_ Framebuffer object for the G-Buffer used in SSAO.
   */
  GLuint g_buffer_FBO_;

  /**
   * @brief initialized_ Whether the widget has finished initializations.
   */
  bool initialized_;

  /**
   * @brief SSAO_enabled_ Whether the SSAO effect is enabled.
   */
  bool SSAO_enabled_;

  /**
   * @brief width_ Viewport current width.
   */
  float width_;

  /**
   * @brief height_ Viewport current height.
   */
  float height_;

  /**
   * @brief currentShader_ Indicates current shader: ( 0 - Phong, 1 - Texture Mapping, 2 - Reflection, 3 - BRDF)
   */
  int currentShader_;

  /**
   * @brief fresnel_ Fresnel F0 color components.
   */
  glm::vec3 fresnel_;

  /**
   * @brief currentTexture_ Indicates the visible texture in texture mapping
   */
  int currentTexture_;

  /**
   * @brief currentSSAORenderMode_ Indicates the current SSAO render mode
   */
  int currentSSAORenderMode_;

  /**
   * @brief currentTexture_ Indicates the visible texture in texture mapping
   */
  bool skyVisible_;

  /**
   * @brief metalness_ Indicates the general metalness properties of the model
   */
  float metalness_;

  /**
   * @brief roughness_ Indicates the general roughness properties of the model
   */
  float roughness_;

  /**
   * @brief albedo_ Albedo color components.
   */
  glm::vec3 albedo_;

  /**
   * @brief useTextures_ Indicates whether to use textures or not when computing the lighting
   */
  bool useTextures_;

  /**
   * @brief applyGammaCorrection_ Indicates whether to apply gamma correction or not
   */
  bool applyGammaCorrection_;

  /**
   * @brief SSAO parameters
   */
  int ssao_num_directions_;
  int ssao_samples_per_direction_;
  float ssao_sample_radius_;
  bool use_randomization_;
  float bias_angle_;            // To reduce tangent surface artifacts
  float ao_strength_;           // AO effect strength
  int ao_algorithm_;            // 0: Spherical Sampling, 1: Horizon Based Ambient Occlusion
  
  bool use_blur_;
  int blur_type_;               // 1:Simple, 2:Bilateral, 3:Gaussian
  float blur_radius_;
  float normal_threshold_;      // For bilateral blur
  float depth_threshold_;       // For bilateral blur

  GLuint VAO;
  GLuint VBO_v;
  GLuint VBO_n;
  GLuint VBO_tc;
  GLuint VBO_i;

  GLuint VAO_sky;
  GLuint VBO_v_sky;
  GLuint VBO_i_sky;
  std::vector<float> skyVertices_;
  std::vector<int> skyFaces_;

  GLuint quad_VAO;
  GLuint quad_VBO;

  GLuint ssao_texture_;
  GLuint ssao_FBO_;
  GLuint blurred_ssao_texture_;
  GLuint blur_FBO_;
  GLuint noise_texture_;



 protected slots:
  /**
   * @brief renderMesh renders the mesh with the given transformation matrices.
   * @param model Model matrix.
   * @param view View matrix.
   * @param projection Projection matrix.
   * @param normal Normal matrix.
   */
  void renderMesh(glm::mat4x4 model, glm::mat4x4 view, glm::mat4x4 projection, glm::mat3 normal);
  
  /**
   * @brief renderSkybox renders the skybox with the given transformation matrices.
   * @param model Model matrix.
   * @param view View matrix.
   * @param projection Projection matrix.
   * @param normal Normal matrix.
   */
  void renderSkybox(glm::mat4x4 model, glm::mat4x4 view, glm::mat4x4 projection, glm::mat3 normal);
  
  /**
   * @brief paintGL Function that handles rendering the scene.
   */
  void paintGL();

  /**
   * @brief renderDefault Renders the scene with the default shader.
   */
  void renderDefault();

  /**
   * @brief renderWithSSAO Renders the scene with the SSAO effect.
   */
  void renderWithSSAO();

  /**
   * @brief SetReflection Enables the reflection shader.
   */
  void SetReflection(bool set);

  /**
   * @brief SetReflection Enables the brdf shader.
   */
  void SetPBS(bool set);

  /**
   * @brief SetReflection Enables the brdf shader.
   */
  void SetIBLPBS(bool set);

  /**
   * @brief SetReflection Enables the reflection shader.
   */
  void SetPhong(bool set);

  /**
   * @brief SetReflection Enables the brdf shader.
   */
  void SetTexMap(bool set);

  /**
   * @brief SetFresnelR Sets the fresnel F0 red component.
   */
  void SetFresnelR(double);

  /**
   * @brief SetFresnelB Sets the fresnel F0 blue component.
   */
  void SetFresnelB(double);

  /**
   * @brief SetFresnelG Sets the fresnel F0 green component.
   */
  void SetFresnelG(double);

  /**
   * @brief SetUseTextures Sets whether to use textures or not.
   */
  void SetUseTextures(bool);

  /**
   * @brief ApplyGammaCorrection Sets whether to apply gamma correction or not.
   */
  void ApplyGammaCorrection(bool);
  
  /**
   * @brief SetCurrentTexture sets the current texture to show
   */
  void SetCurrentTexture(int);

  /**
   * @brief SetCurrentTexture sets the current texture to show
   */
  void SetSkyVisible(bool set);

  /**
   * @brief SetFaces Signal that updates the interface label "Framerate".
   */
  void SetMetalness(double);

  /**
   * @brief SetFaces Signal that updates the interface label "Framerate".
   */
  void SetRoughness(double);

   /**
   * @brief SetSSAODirections Sets the number of directions for SSAO sampling
   */
  void SetSSAODirections(int directions);

  /**
   * @brief SetSSAOSamplesPerDirection Sets the number of samples per direction for SSAO
   */
  void SetSSAOSamplesPerDirection(int samples);

  /**
   * @brief SetSSAORadius Sets the sampling radius for SSAO
   */
  void SetSSAORadius(double radius);

  /**
   * @brief SetSSAORenderMode Sets the current SSAO render mode
   */
  void SetSSAORenderMode(int mode);

  /**
   * @brief EnableSSAO Enables or disables SSAO rendering
   */
  void EnableSSAO(bool enable);

  /**
   * @brief SetUseRandomization Sets whether to use randomization in SSAO sampling
   * @param use Whether to use randomization
   */
  void SetUseRandomization(bool use);

  /**
   * @brief SetBasicSSAO Sets the basic SSAO algorithm
   */
  void SetBasicSSAO(bool set);

  /**
   * @brief SetHBAO Sets the Horizon Based Ambient Occlusion algorithm
   * @param set Whether to use HBAO
   */
  void SetHBAO(bool set);

  /**
   * @brief SetUseBlur Sets whether to use blurring in SSAO Final Composition.
   * @param use Whether to use blurring
   */
  void SetUseBlur(bool use);

  /**
   * @brief SetBlurType Sets the type of blur to use in blur SSAO
   * @param type The type of blur (1: Simple, 2: Bilateral, 3: Gaussian)
   */
  void SetBlurType(int type);

  /**
   * @brief SetBlurRadius Sets the radius of the blur in blur SSAO
   * @param radius The radius of the blur
   */
  void SetBlurRadius(double radius);

  /**
   * @brief SetNormalThreshold Sets the normal threshold for bilateral blur
   * @param threshold The normal threshold value
   */
  void SetNormalThreshold(double threshold);

  /**
   * @brief SetDepthThreshold Sets the depth threshold for bilateral blur
   * @param threshold The depth threshold value
   */
  void SetDepthThreshold(double threshold);

  /**
   * @brief SetBiasAngle Sets the bias angle for SSAO
   * @param angle The bias angle value
   */
  void SetBiasAngle(double angle);

  /**
   * @brief SetAOStrength Sets the ambient occlusion strength
   * @param strength The ambient occlusion strength value
   */
  void SetAOStrength(double strength);

 signals:
  /**
   * @brief SetFaces Signal that updates the interface label "Faces".
   */
  void SetFaces(QString);

  /**
   * @brief SetFaces Signal that updates the interface label "Vertices".
   */
  void SetVertices(QString);

  /**
   * @brief SetFaces Signal that updates the interface label "Framerate".
   */
  void SetFramerate(QString);


};

#endif  //  GLWIDGET_H_
