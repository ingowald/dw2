// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "glfwWindow.h"
#include <chrono>
#include <sstream>

namespace dw2 {

  GLFWindow::GLFWindow(const vec2i &size, const vec2i &position, const std::string &title,
                       bool doFullScreen, bool stereo, int monitorID)
    : size(size),
      position(position),
      title(title),
      monitorID(monitorID),
      stereo(stereo),
      doFullScreen(doFullScreen)
  {
    create();
  }


  void GLFWindow::create()
  {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
      
    if (doFullScreen) {
      auto *monitor = glfwGetPrimaryMonitor();
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      // const vec2i screenSize = getScreenSize();
      glfwWindowHint(GLFW_AUTO_ICONIFY,false);
      glfwWindowHint(GLFW_RED_BITS, mode->redBits);
      glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
      glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
      glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        
      this->handle = glfwCreateWindow(mode->width, mode->height,
                                      title.c_str(), monitor, nullptr);
    } else {
      int numMonitors;
      GLFWmonitor **monitors = glfwGetMonitors(&numMonitors);
      PRINT(numMonitors);
      if (monitorID >= numMonitors) {
        std::stringstream err;
        err << "glfw has only " << numMonitors
            << " monitors, but config requested to open on display #"
            << monitorID;
        throw std::runtime_error(err.str());
      }
      const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorID]);
      glfwWindowHint(GLFW_RESIZABLE,0);
      glfwWindowHint(GLFW_DECORATED,0);
      this->handle = glfwCreateWindow(size.x,size.y,title.c_str(),
                                      NULL,NULL);
      glfwSetWindowPos(this->handle, position.x, position.y);
    }

    glfwMakeContextCurrent(this->handle);
  }

  void GLFWindow::setFrameBuffer(FrameBuffer::SP newFB)
  {
    assert(newFB);
    assert(newFB->size == this->size);
    std::lock_guard<std::mutex> lock(fbMutex);
    currentFB = newFB;
  }

  FrameBuffer::SP GLFWindow::getFrameBuffer()
  {
    std::unique_lock<std::mutex> lock(fbMutex);
    return currentFB;
  }

  void GLFWindow::display() 
  {    
    vec2i currentSize(0,0);
    glfwGetFramebufferSize(this->handle, &currentSize.x, &currentSize.y);
    
    glViewport(0, 0, currentSize.x, currentSize.y);
    glClear(GL_COLOR_BUFFER_BIT);

    FrameBuffer::SP displayFB = getFrameBuffer();
    if (!displayFB) {
      usleep(1000);
      glfwSwapBuffers(this->handle);
      return;
    }

    // no stereo for now:
    assert(!displayFB->leftEyePixels.empty());
    assert(displayFB->rightEyePixels.empty());

    glDrawPixels(displayFB->size.x, displayFB->size.y, GL_RGBA, GL_UNSIGNED_BYTE,
                 displayFB->leftEyePixels.data());
    glfwSwapBuffers(this->handle);
    
    if (displayFB == lastDisplayedFB) {
      usleep(1000);
    } else {
      lastDisplayedFB = displayFB;
    }
  }

  vec2i GLFWindow::getSize() const 
  { 
    return size; 
  }

  bool GLFWindow::doesStereo() const
  { 
    return stereo; 
  }
    
  void GLFWindow::run() 
  { 
    while (!glfwWindowShouldClose(this->handle)) {
      glfwPollEvents();
      display();

      //        usleep(1000);
    }
  }
    
} // ::dw2
