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
   * @brief LoadDiffuseMap Will load load a cube map that will be used for the
   * specular component.
   * @param filename Path to the directory containing the 6 textures (right,
   * left, top, bottom, front back) of the sube map that will be used for the
   * diffuse component.
   * @return Whether it was able to load the textures.
   */
  bool LoadDiffuseMap(const QString &filename);

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

 protected:
  /**
   * @brief initializeGL Initializes OpenGL variables and loads, compiles and
   * links shaders.
   */
  void initializeGL();

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
   * @brief initialized_ Whether the widget has finished initializations.
   */
  bool initialized_;

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


 protected slots:
  /**
   * @brief paintGL Function that handles rendering the scene.
   */
  void paintGL();

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
