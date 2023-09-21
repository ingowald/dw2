## Copyright (c) 2016 Ingo Wald
## 
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
## 
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.

# cmake_minimum_required(VERSION 3.9)

project(dw2ClientLib VERSION 1.0.0)

set (CMAKE_CXX_STANDARD 11)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

# ------------------------------------------------------------------
# the actual client library
# ------------------------------------------------------------------
add_library(dw2_client
  dw2_client.cpp
)

target_compile_options(dw2_client PRIVATE
  )

target_compile_options(dw2_client PRIVATE
  -fPIC
  )

target_link_libraries(dw2_client PUBLIC
  dw2_common
  )
#target_link_libraries(dw2_client PRIVATE
#  dw2_common
#  TBB::tbb
# )

# include(GNUInstallDirs)

# set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/dw2_client)

# target_include_directories(dw2_client PUBLIC
# 	$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
# )

# install(TARGETS dw2_client
#     EXPORT dw2_clientTargets 
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )

# set_target_properties(dw2_client PROPERTIES EXPORT_NAME dw2_client)

# install(EXPORT dw2_clientTargets
#   FILE
#     dw2_clientTargets.cmake
#   NAMESPACE
#     dw2_client::
#   DESTINATION
#     ${INSTALL_CONFIGDIR}
# )

# install(
#   FILES
#     dw2_client.h
#   DESTINATION
#     ${CMAKE_INSTALL_INCLUDEDIR}
# )

# #Create a ConfigVersion.cmake file
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#     ${CMAKE_CURRENT_BINARY_DIR}/dw2_clientConfigVersion.cmake
#     VERSION ${PROJECT_VERSION}
#     COMPATIBILITY AnyNewerVersion
# )

# configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/dw2_clientConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/dw2_clientConfig.cmake
#     INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
# )

# #Install the config, configversion and custom find modules
# install(FILES
#     ${CMAKE_CURRENT_BINARY_DIR}/dw2_clientConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/dw2_clientConfigVersion.cmake
#     DESTINATION ${INSTALL_CONFIGDIR}
# )

# ##############################################
# ## Exporting from the build tree

# export(EXPORT dw2_clientTargets FILE ${CMAKE_CURRENT_BINARY_DIR}/dw2_clientTargets.cmake NAMESPACE dw2_client::)

# #Register package in user's package registry
# # export(PACKAGE dw2_client)




