# ViewerPBS

This is a skeleton viewer developed for the **Fast Realistic Rendering** course in the **Master in Research in Informatics** program at the **Universitat Politècnica de Catalunya**.

## Authors

The skeleton was created by **Imanol Munoz-Pandilella** (imanolm@cs.upc.edu), based on a previous skeleton by **Marc Comino**.

## License

This work is protected under a [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/) license.

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

