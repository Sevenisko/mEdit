cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ROOT_DIR}/Lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR}/Bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DIR}/Bin)

project(mEdit VERSION "0.0.1")

add_library(ImGui STATIC 
    "${ROOT_DIR}/Vendors/ImGui/imgui.cpp" 
    "${ROOT_DIR}/Vendors/ImGui/imgui_draw.cpp" 
    "${ROOT_DIR}/Vendors/ImGui/imgui_tables.cpp" 
    "${ROOT_DIR}/Vendors/ImGui/imgui_widgets.cpp" 
    "${ROOT_DIR}/Vendors/ImGui/imgui_stdlib.cpp" 
 
    "${ROOT_DIR}/Vendors/ImGui/imgui_impl_win32.cpp"
    "${ROOT_DIR}/Vendors/ImGui/imgui_impl_dx9.cpp"
    "${ROOT_DIR}/Vendors/ImGui/imgui_impl_dx8.cpp"
    "${ROOT_DIR}/Vendors/ImGui/imgui_impl_dinput.cpp"
    "${ROOT_DIR}/Vendors/ImGui/ImGuizmo.cpp"
    "${ROOT_DIR}/Vendors/ImGui/ImSequencer.cpp"
    "${ROOT_DIR}/Vendors/ImGui/TextEditor.cpp")

add_library(lodepng STATIC 
    "${ROOT_DIR}/Vendors/lodepng/lodepng.cpp" 
)

configure_file("${ROOT_DIR}/Config.tmpl" "${ROOT_DIR}/Include/Config.h")
configure_file("${ROOT_DIR}/mEdit-tmpl.rc" "${ROOT_DIR}/mEdit.rc")

include_directories("Include" "Vendors/LS3D/Include" "Vendors/DirectX/Include" "Vendors/ImGui" "Vendors/lodepng")
link_directories("Vendors/LS3D/Lib" "Vendors/DirectX/Lib")

add_compile_options(/utf-8)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS GLM_ENABLE_EXPERIMENTAL)

add_executable(mEdit WIN32 
    "mEdit.rc"

    "Source/Main.cpp"
    "Source/EditorApplication.cpp"
    
    "Source/Data/TextDatabase.cpp"
    "Source/Data/ItemsDatabase.cpp"
    
    "Source/IO/FileSystem.cpp"

    "Source/Editors/TextEditor.cpp"
    "Source/Editors/SceneEditor.cpp" 
    "Source/Cache.cpp" )

file(GLOB LS3D_BIN_FILES "${ROOT_DIR}/Vendors/LS3D/Bin/*")
foreach(FILE ${LS3D_BIN_FILES})
    get_filename_component(FILENAME ${FILE} NAME)
    add_custom_command(TARGET mEdit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${FILENAME}"
        COMMENT "Copying ${FILENAME} to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
endforeach()

target_link_libraries(mEdit LS3DF rw_data ImGui lodepng gdi32 d3d8 d3d9 dinput8 dxguid comctl32 UxTheme)
