# mEdit - Another Mafia Editor

mEdit is an all-in-one editor suite for modding of Mafia: TCoLH.

## Goal of the project

The goal is to make a project, which would be able to modify all known assets of Mafia: The City of Lost Heaven, which is running on LS3D engine.

Here are the following features within mEdit:

- [ ] Scene Editor
  - [x] Scene loading and saving
  - [x] Frame editing (create/destroy/modify)
  - [x] Actor management (create/destroy/modify)
  - [x] Road.bin editing
  - [x] Check.bin editing
  - [x] Possibility to modify cache.bin contents
  - [ ] Ability to create difference files (*.chg)
  - [x] Built-in script editor
  - [ ] Collision editor
  - [ ] Lightmap baking
- [ ] Text Editor
  - [ ] Loading and saving TextDB files
  - [ ] Conversion from Windows-1250 to UTF-8 and vice versa
  - [ ] Text entry modification
- [ ] Item Editor
  - [ ] Loading and saving predmety.def
  - [ ] Ability to modify the contents
- [ ] Vehicle Editor
  - [ ] Loading and saving appropriate files
  - [ ] Ability to modify certain parameters
  - [ ] Ability to create custom brands or vehicles
- [ ] Cutscene Editor
  - [ ] Loading and saving .rep files
  - [ ] Ability to animate camera movement
  - [ ] Ability to manipulate the timeline
- [ ] Asset Converter
  - [ ] Ability to convert to 4ds from common formats (for example IQM, FBX)
  - [ ] Ability to convert textures to appropriate format (from common formats, like PNG)

## Building the project

In order to build the project, you must have Visual Studio with C++ components installed (2022 is recommended) and CMake (is usually bundled with VS).

There are two ways to build the project:    

- Through command prompt:
  
  ```
      cd <mEditSourceDir>
      mkdir build && cd build
      cmake ..
      cmake --build .
  ```
- Through CMake-compatible editor: Many IDEs have built-in support for CMake (as long as you have MSVC build tools, you can use any IDE / Code editor), you can simply build mEdit.exe target.

## Contributing to the project

In order to contribute to mEdit, you'll need to be able to work with C++ (obviously). Also you'll need some knowledge about inner workings of the LS3D engine (since mEdit uses LS3D in some cases). And most of all, you'll need some logical thinking (you're allowed to use AI, but I prefer manually-written code).

The workflow of contribution is simple:

1. Clone the repository
2. Create a new branch for the feature or fix
3. Make and push the changes
4. Create a pull request

## 3rd-Party software used

- [Dear ImGui](https://github.com/ocornut/imgui) for the user interface
- LS3D engine for 3D rendering
- [LodePNG](https://github.com/lvandeve/lodepng) currently just for the icon, but will be used in the future for easy texture conversion