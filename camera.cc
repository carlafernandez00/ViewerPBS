// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <camera.h>

#define GLM_FORCE_RADIANS

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>


#include <algorithm>
#include <cmath>

namespace data_visualization {

namespace {

const glm::vec3 vra(0.0, 1.0, 0.0);
const glm::vec3 hra(1.0, 0.0, 0.0);

}  //  namespace

Camera::Camera()
    : distance_(2.0),
      step_(0.05),
      rotating_(false),
      zooming_(false),
      panning_(false),
      rotation_y_(0.0),
      rotation_x_(0.0),
      pan_x_(0.0),
      pan_y_(0.0),
      current_x_(-1.0),
      current_y_(-1.0),
      viewport_x_(0.0),
      viewport_y_(0.0),
      viewport_width_(0.0),
      viewport_height_(0.0),
      centering_x_(0.0),
      centering_y_(0.0),
      centering_z_(0.0),
      scaling_(1.0),
      field_of_view_(0.0),
      z_near_(0.0),
      z_far_(0.0) {}

void Camera::SetViewport(int x, int y, int w, int h) {
  viewport_x_ = x;
  viewport_y_ = y;
  viewport_width_ = w;
  viewport_height_ = h;

  glViewport(viewport_x_, viewport_y_, viewport_width_, viewport_height_);
}

void Camera::SetViewport() const {
  glViewport(viewport_x_, viewport_y_, viewport_width_, viewport_height_);
}

glm::mat4 Camera::SetIdentity() const {
  return glm::mat4(1.0);
}

glm::mat4 Camera::SetModel() const {

   glm::mat4 kScaling = glm::scale(glm::mat4(1.), glm::vec3(scaling_, scaling_, scaling_));
   glm::mat4 kTranslation = glm::translate(glm::mat4(1.), glm::vec3(centering_x_, centering_y_, centering_z_));

  return kScaling * kTranslation;
}

glm::mat4 Camera::SetView() const {
    glm::mat4 kTranslation = glm::translate(glm::mat4(1.), glm::vec3(pan_x_, pan_y_, -distance_));
    glm::mat4 kRotationA = glm::rotate(glm::mat4(1.), (float)rotation_x_, hra);
    glm::mat4 kRotationB = glm::rotate(glm::mat4(1.), (float)rotation_y_, vra);

  return kTranslation * kRotationA * kRotationB;
}

glm::mat4 Camera::SetProjection(double fov, double znear, double zfar) {
  field_of_view_ = fov;
  z_near_ = znear;
  z_far_ = zfar;

  return SetProjection();
}

glm::mat4 Camera::SetProjection() const {
  const double kAR = static_cast<double>(viewport_width_) /
                     static_cast<double>(viewport_height_);

  return glm::perspective((field_of_view_ * M_PI / 180.0), kAR, z_near_, z_far_);
}

void Camera::Zoom(double modifier) {
  distance_ += step_ * modifier;

  if (distance_ < kMinCameraDistance) distance_ = kMinCameraDistance;
  if (distance_ > kMaxCameraDistance) distance_ = kMaxCameraDistance;
}

void Camera::SafeZoom(double modifier) {
  if (zooming_) {
    Zoom(modifier - current_y_ < 0 ? -1 : 1);
    current_y_ = modifier;
  }
}

void Camera::SafePan(double x, double y) {
  if (panning_) {
    pan_x_ += (x - current_x_) / 10.0 * step_;
    pan_y_ -= (y - current_y_) / 10.0 * step_;
    current_y_ = y;
    current_x_ = x;
  }
}

void Camera::Rotate(double modifier) {
  rotation_y_ += AngleIncrement * modifier;
}

void Camera::UpdateModel(glm::vec3 min, glm::vec3 max) {
  glm::vec3 center = (min + max) / 2.f;
  centering_x_ = -center[0];
  centering_y_ = -center[1];
  centering_z_ = -center[2];

  float longest_edge =
      std::max(max[0] - min[0], std::max(max[1] - min[1], max[2] - min[2]));
  scaling_ = 1.0 / static_cast<double>(longest_edge);
}

void Camera::SetRotationX(double y) {
  if (rotating_) {
    rotation_x_ += (y - current_y_) * step_;
    current_y_ = y;
    if (rotation_x_ < kMinRotationX) rotation_x_ = kMinRotationX;
    if (rotation_x_ > MaxRotationX) rotation_x_ = MaxRotationX;
  }
}

void Camera::SetRotationY(double x) {
  if (rotating_) {
    rotation_y_ += (x - current_x_) * step_;
    current_x_ = x;
  }
}

void Camera::StartRotating(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  rotating_ = true;
}

void Camera::StopRotating(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  rotating_ = false;
}

void Camera::StartZooming(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  zooming_ = true;
}

void Camera::StopZooming(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  zooming_ = false;
}

void Camera::StartPanning(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  panning_ = true;
}

void Camera::StopPanning(double x, double y) {
  current_x_ = x;
  current_y_ = y;
  panning_ = false;
}

void Camera::SetCameraStep(double step) { this->step_ = step; }

glm::vec3 Camera::GetPosition() const {
  // The camera position can be extracted from the inverse of the view matrix
  // The view matrix is the transformation from world space to camera space
  // Its inverse transforms from camera space to world space
  // The camera is at origin (0,0,0) in camera space, so transforming this point
  // gives us the camera position in world space
  
  glm::mat4 view = SetView();
  glm::mat4 viewInverse = glm::inverse(view);
  
  // Extract the translation component (camera position)
  return glm::vec3(viewInverse[3]);
}

}  //  namespace data_visualization
