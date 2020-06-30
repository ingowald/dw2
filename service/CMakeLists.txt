# ======================================================================== #
# Copyright 2019 Ingo Wald                                                 #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http://www.apache.org/licenses/LICENSE-2.0                           #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")


# ------------------------------------------------------------------
# the 'core' service library that implements everything up the point
# of assembling final frames on the display nodes, but _without_ the
# actual app code that takes such received frames and puts them on a
# screen.
# ------------------------------------------------------------------

add_library(dw2_core_service
  FrameAssembler.cpp
#  main.cpp
# glfwWindow.cpp
  Dispatcher.cpp
  Server.cpp
  InfoServer.cpp
  )

target_include_directories(dw2_core_service PRIVATE
  ${MPI_CXX_INCLUDE_PATH}
  )

target_compile_options(dw2_core_service PRIVATE
  ${MPI_CXX_COMPILE_FLAGS}
  )

target_compile_options(dw2_core_service PRIVATE
  -fPIC
  )

target_link_libraries(dw2_core_service PUBLIC
  dw2_common
  )

target_link_libraries(dw2_core_service PRIVATE
  ${MPI_CXX_LIBRARIES}
  ${MPI_CXX_LINK_FLAGS}
  )


# ------------------------------------------------------------------
# the actual service that receives full frames and displays them in a
# gl/glfw window
# ------------------------------------------------------------------

add_executable(dw2_service
#  FrameAssembler.cpp
   main.cpp
   glfwWindow.cpp
#  Dispatcher.cpp
#  Server.cpp
#  InfoServer.cpp
  )

target_include_directories(dw2_service PRIVATE
  ${MPI_CXX_INCLUDE_PATH}
  )

target_compile_options(dw2_service PRIVATE
  ${MPI_CXX_COMPILE_FLAGS}
  )

target_compile_options(dw2_service PRIVATE
  -fPIC
  )

target_link_libraries(dw2_service PRIVATE
  dw2_core_service
  glfw
  ${OPENGL_LIBRARIES}
  )

