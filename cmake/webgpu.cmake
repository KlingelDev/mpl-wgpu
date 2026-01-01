# cmake/webgpu.cmake - WebGPU integration for mpl-wgpu
# Pattern adapted from OTC.SDK.Visualization

cmake_policy(SET CMP0169 OLD)

# Fetch WebGPU-distribution for C++ wrapper and headers
FetchContent_Declare(webgpu_dist
  GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
  GIT_TAG main
)

FetchContent_GetProperties(webgpu_dist)
if(NOT webgpu_dist_POPULATED)
  FetchContent_Populate(webgpu_dist)
endif()

message(STATUS "webgpu_dist populated: ${webgpu_dist_SOURCE_DIR}")

# Fetch wgpu-native
FetchContent_Declare(wgpu_native
  GIT_REPOSITORY https://github.com/gfx-rs/wgpu-native.git
  GIT_TAG v0.19.4.1
)

FetchContent_GetProperties(wgpu_native)
if(NOT wgpu_native_POPULATED)
  FetchContent_Populate(wgpu_native)
  
  # Build wgpu-native
  message(STATUS "Building wgpu-native...")
  execute_process(
    COMMAND make lib-native-release
    WORKING_DIRECTORY ${wgpu_native_SOURCE_DIR}
    RESULT_VARIABLE WGPU_BUILD_RESULT
  )
  
  if(NOT WGPU_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build wgpu-native")
  endif()
endif()

# Create imported library target
add_library(wgpu_native_lib STATIC IMPORTED)
set_target_properties(wgpu_native_lib PROPERTIES
  IMPORTED_LOCATION 
    "${wgpu_native_SOURCE_DIR}/target/release/libwgpu_native.a"
  INTERFACE_INCLUDE_DIRECTORIES "${wgpu_native_SOURCE_DIR}/ffi"
)

# Create 'webgpu' interface target
if(NOT TARGET webgpu)
  add_library(webgpu INTERFACE)
  target_link_libraries(webgpu INTERFACE wgpu_native_lib)
  target_include_directories(webgpu INTERFACE
    "${webgpu_dist_SOURCE_DIR}/wgpu-native/include"
    "${wgpu_native_SOURCE_DIR}/ffi"
    "${wgpu_native_SOURCE_DIR}/ffi/webgpu-headers"
  )
  target_compile_definitions(webgpu INTERFACE WEBGPU_BACKEND_WGPU)
endif()

message(STATUS "WebGPU configured successfully")
