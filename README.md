# Fast Realistic Rendering - SSAO Framework

A real-time rendering framework implementing Physically Based Rendering (PBR) with Image-Based Lighting (IBL) and Screen-Space Ambient Occlusion (SSAO) techniques.

## Overview

This project extends a PBR rendering framework with advanced ambient occlusion capabilities. It implements a multi-pass rendering pipeline that computes ambient occlusion in screen space to enhance visual realism by simulating proximity shadows and depth-based shading effects.

## Features

### Rendering Techniques

- **Physically Based Rendering (PBR)** with metallic-roughness workflow
- **Image-Based Lighting (IBL)** using environment maps and precomputed BRDF lookup tables
- **Screen-Space Ambient Occlusion (SSAO)** with multiple algorithms
- **Horizon-Based Ambient Occlusion (HBAO)** implementation
- **Multi-pass deferred rendering** pipeline
- **HDR Environment Mapping** with tone mapping and gamma correction

### PBR & IBL Features

- **Metallic-Roughness Workflow**: Industry-standard material model
- **Environment-Based Lighting**: Diffuse irradiance and specular reflection from HDR cube maps
- **BRDF Integration**: Pre-computed lookup tables for real-time evaluation
- **Fresnel Calculations**: Accurate reflection behavior with F0 control
- **Multiple Material Inputs**: Support for albedo, roughness, metalness, and normal maps
- **Gamma Correction**: Linear workflow with sRGB output conversion
- **Tone Mapping**: HDR to LDR conversion for display

### SSAO Implementation

- **G-Buffer Generation**: Renders albedo, normals, and depth to textures
- **Ambient Occlusion Calculation**: Samples surrounding fragments to estimate occlusion
- **Noise-Based Randomization**: Reduces banding artifacts with random sampling
- **Post-Processing Blur**: Multiple blur types (Simple, Bilateral, Gaussian)
- **Real-time Parameter Adjustment**: Interactive GUI controls

### Supported Shading Models

- **Phong Shading**: Classic lighting model for basic illumination
- **Texture Mapping**: Material property visualization and testing
- **Reflection Mapping**: Environment reflections using cube maps
- **Simple PBR**: Basic Cook-Torrance BRDF implementation
- **IBL PBR**: Advanced PBR with split-sum approximation for environment lighting
  - Diffuse irradiance integration
  - Specular prefiltered environment maps
  - BRDF integration lookup tables


## Requirements

The skeleton is designed to be developed on **Linux** and requires the following libraries:
- [Qt](https://www.qt.io) >= 6.0.0
- [GLM](https://github.com/g-truc/glm)

While we do not provide support for other platforms, the skeleton **should work** on **Windows** and **macOS**.

### Installation Instructions

#### General Libraries and Drivers

A **GCC compiler** and **OpenGL drivers** are required:
```sh
sudo apt update
sudo apt install build-essential libgl1-mesa-dev
```

#### Qt Installation
1. Download the [Qt installer](https://www.qt.io/download-qt-installer-oss).
2. Make the installer executable and run it:
   ```sh
   chmod +x qt-online-installer-linux-x64-4.8.1.run
   ./qt-online-installer-linux-x64-4.8.1.run
   ```
3. Follow the installation steps and select **Qt 6.X for Desktop Development**.
4. If necessary, add the installation folder to your system `PATH`.

#### GLM Installation
Install GLM using the following command:
```sh
sudo apt install libglm-dev
```

## Compilation

You can compile the project using the following commands:
```sh
qmake
make
```
Alternatively, you can use **Qt Creator IDE** (included with the Qt installation) to simplify the process and take advantage of its features.

## Running on macOS

If you're using macOS, after building the application, you need to set up a symbolic link to the shaders and textures directory:

```sh
cd release/ViewerPBS.app/Contents/MacOS
ln -s ../../../shaders ../shaders
ln -s ../../../textures ../textures
ln -s ../../../models ../models
```

This ensures that the application can find the shader files when running on macOS.

## Usage

### Loading Models
- **File → Load Model**: Support for PLY and OBJ formats
- Default sphere geometry loads automatically
- Automatic normal generation and bounding box computation

### Loading Textures & Environment Maps

#### PBR Material Maps
- **File → Load Color**: Albedo/diffuse color maps (sRGB)
- **File → Load Roughness**: Surface roughness maps (linear)
- **File → Load Metalness**: Metallic/non-metallic classification maps (linear)

#### IBL Environment Maps
- **File → Load Specular**: Environment cube map for reflections (HDR recommended)
- **File → Load Diffuse**: Pre-computed irradiance maps for diffuse lighting
- **File → Load Weighted Specular**: Prefiltered environment maps with mip levels
- **File → Load BRDF LUT**: 2D lookup table for BRDF integration

#### Material Properties
- **Albedo Color Picker**: Base color selection when not using texture maps
- **Roughness Slider** (0.0-1.0): Surface microsurface detail
- **Metalness Slider** (0.0-1.0): Metallic vs dielectric material classification
- **Fresnel F0 RGB**: Base reflectance for dielectric materials
- **Use Texture Maps**: Toggle between procedural and texture-based materials
- **Apply Gamma Correction**: Enable sRGB output conversion

### SSAO Controls

#### Algorithm Selection
- **Basic SSAO**: Standard screen-space sampling
- **HBAO**: Horizon-based ambient occlusion (more accurate)

#### Sampling Parameters
- **Directions** (4-64): Number of sampling directions
- **Samples** (1-16): Samples per direction
- **Radius** (0.01-2.0): Sampling radius in view space
- **Bias Angle** (0.0-0.5): Reduces self-occlusion artifacts

#### Quality Enhancement
- **Use Randomization**: Reduces banding with noise texture
- **Use Blur**: Enables post-processing blur
- **Blur Type**: Simple/Bilateral/Gaussian options
- **AO Strength** (0.0-1.0): Final occlusion intensity

#### Visualization Modes
- **Albedo**: Base color only
- **Normals**: View-space normals (RGB encoded)
- **Depth**: Linear depth visualization
- **SSAO**: Raw ambient occlusion
- **Blurred SSAO**: Post-processed occlusion
- **Final Composition**: Combined result

### Camera Controls
- **Left Mouse**: Rotate camera around model
- **Right Mouse**: Zoom in/out
- **Arrow Keys/WASD**: Alternative camera movement
- **R Key**: Reload all shaders (for development)