cmake_minimum_required(VERSION 3.16)


project (
    JCEngine
    VERSION 0.1
    DESCRIPTION "My First Easy Engine"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(SHADERC_BIN ${PROJECT_SOURCE_DIR}/tools/bin/windows/shadercRelease.exe)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

include_directories(include)

aux_source_directory(src/subsys SUBSYS_SOURCE)
aux_source_directory(src/imgui IMGUI_SOURCE)

set(SHADER_PATH src/shaders)

function(compile_shader path)
    execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${SHADER_PATH})

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${SHADER_PATH}/${path}.vert.spv
        COMMAND ${CMAKE_SOURCE_DIR}/tools/glslang.exe 
            ${CMAKE_SOURCE_DIR}/${SHADER_PATH}/${path}.vert 
            -o ${CMAKE_BINARY_DIR}/${SHADER_PATH}/${path}.vert.spv -V
        COMMENT "Compile ${SHADER_PATH}/${path} VERT"
    )

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/${SHADER_PATH}/${path}.frag.spv
        COMMAND ${CMAKE_SOURCE_DIR}/tools/glslang.exe 
            ${CMAKE_SOURCE_DIR}/${SHADER_PATH}/${path}.frag 
            -o ${CMAKE_BINARY_DIR}/${SHADER_PATH}/${path}.frag.spv -V
        COMMENT "Compile ${path} VERT"
    )

    add_custom_target(${path} ALL DEPENDS ${SHADER_PATH}/${path}.vert.spv ${SHADER_PATH}/${path}.frag.spv)
endfunction()

compile_shader(shader)

add_executable(hello src/main.cpp ${SUBSYS_SOURCE})
target_link_libraries(hello PRIVATE SDL3_image::SDL3_image SDL3::SDL3)
add_dependencies(hello shader)
# target_compile_definitions(hello PRIVATE -DDEBUG)