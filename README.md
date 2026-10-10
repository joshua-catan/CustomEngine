# mestEngine

**mestEngine** is a small custom game-engine project written in C++ using OpenGL, GLFW, GLEW, and Dear ImGui. The current project focuses on building a simple editor with a scene view and basic game-object controls.

> **Project status:** Early development. Features and architecture may change as the engine grows.

## Current Features

- C++ application with an OpenGL rendering context
- GLFW window and input handling
- GLEW for OpenGL extension loading
- Dear ImGui editor interface
- Hierarchy panel for listing scene objects
- Inspector panel for editing object transforms
- Scene panel that renders 3D cubes
- Mouse-drag rotation for the selected object
- Create Cube and Create Light buttons
- Delete Selected button
- Console panel showing basic engine status and object count
- CMake build configuration

## Built With

- **C++**
- **OpenGL**
- **GLFW**
- **GLEW**
- **Dear ImGui**
- **CMake**
- **Ninja** (build generator used in the current setup)
- **MinGW-w64 / MSYS2 UCRT64** (compiler toolchain used in the current setup)

## Project Structure

The project currently follows this layout:

```text
Engine/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── engine/
│   │   ├── GameObject.h
│   │   ├── Renderer.h
│   │   ├── Renderer.cpp
│   │   ├── Scene.h
│   │   └── Scene.cpp
│   └── external/
│       └── imgui/
│           ├── imgui.cpp
│           ├── imgui_draw.cpp
│           ├── imgui_tables.cpp
│           ├── imgui_widgets.cpp
│           └── backends/
│               ├── imgui_impl_glfw.cpp
│               └── imgui_impl_opengl3.cpp
└── build/                 # Generated build files; usually not committed
```

Your exact directory structure may differ slightly depending on how Dear ImGui is included in the repository.

## Requirements

To build the project, install:

1. A C++ compiler that supports C++17 or later.
2. CMake.
3. Ninja, if you use the Ninja generator.
4. GLFW.
5. GLEW.
6. OpenGL development libraries and drivers.
7. Dear ImGui source files included in `src/external/imgui/`.

The commands below assume Windows, MSYS2 UCRT64, and a project directory containing `CMakeLists.txt`.

## Build on Windows

### 1. Open the project folder

Open the project folder in VS Code and open its terminal.

### 2. Configure with CMake

If your project uses the Ninja generator and the MSYS2 UCRT64 compiler, run:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/c++.exe
```

If the `build` directory was already configured, you can usually skip configuration unless your CMake setup changes.

### 3. Build

```powershell
cmake --build build
```

### 4. Run

```powershell
.\build\MyEngine.exe
```

If your executable has a different name in `CMakeLists.txt`, use that name instead.

## Using the Editor

- **Hierarchy:** Select an object or create/delete objects.
- **Inspector:** Edit the selected object's name, position, rotation, and scale.
- **Scene:** View the rendered scene. Select an object in the Hierarchy, then drag with the left mouse button over the scene image to rotate it.
- **Console:** View basic engine status and the number of scene objects.

The current editor layout uses fixed-position panels. Full Unity-style docking and rearranging panels may be added later.

## Current Limitations

This is an early learning and development project, so it is not yet a complete game engine. Depending on the current implementation, limitations may include:

- Basic cube rendering rather than a complete model/material pipeline
- Limited lighting and scene rendering
- No asset browser or scene serialization yet
- No full physics, animation, audio, or scripting system yet
- Fixed editor panels instead of full docking support
- No guarantee of compatibility with every compiler or operating system

## Roadmap

Possible future improvements:

- [ ] Unity-style dockable editor panels
- [ ] Camera navigation controls for the Scene view
- [ ] More primitive shapes
- [ ] Material and color editing
- [ ] Asset browser
- [ ] Save/load scenes
- [ ] Improved rendering and lighting
- [ ] Build and packaging workflow

## Contributing

This project is currently being developed as a personal learning project. Suggestions, bug reports, and contributions are welcome.

When reporting a bug, include:
- Your operating system
- Compiler and CMake versions
- Steps to reproduce the issue
- The full error message, if available

## License

No license has been specified yet. Unless a license is added to this repository, all rights remain with the copyright holder, and reuse, modification, and redistribution are not automatically granted.

If you plan to make this project open source, consider adding a license file and confirming the license terms for all third-party dependencies, including Dear ImGui, GLFW, and GLEW.

---

Made by **joshua**.
