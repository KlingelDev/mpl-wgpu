# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-src")
  file(MAKE_DIRECTORY "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-src")
endif()
file(MAKE_DIRECTORY
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-build"
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix"
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/tmp"
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/src/webgpu_dist-populate-stamp"
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/src"
  "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/src/webgpu_dist-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/src/webgpu_dist-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/karl/prsn/mpl-wgpu/build/_deps/webgpu_dist-subbuild/webgpu_dist-populate-prefix/src/webgpu_dist-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
