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

#pragma once

#include "FrameBuffer.h"
// windowing stuff
#include "GLFW/glfw3.h"
// std
#include <mutex>
#include <condition_variable>

namespace dw2 {

  struct GLFWindow 
  {
    GLFWindow(const vec2i &size, const vec2i &position, const std::string &title,
              bool doFullScreen, bool stereo, int monitorID);

    virtual ~GLFWindow()
    {
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
      PING;
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
      std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << "\n";
    }

    /*! thread-safe setter for the frame buffer to display */
    void setFrameBuffer(FrameBuffer::SP newFB);

    /*! thread-safe getter for the frame buffer to display */
    FrameBuffer::SP getFrameBuffer();
    
    void display(); 
    vec2i getSize()   const;
    bool doesStereo() const;
    void run();
    void create();

    static vec2i getScreenSize() 
    {
      const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
      return vec2i(mode->width,mode->height);
    }

    std::mutex      fbMutex;
    FrameBuffer::SP currentFB;
    FrameBuffer::SP lastDisplayedFB;
    
    GLFWwindow     *handle { nullptr };

    /*! the X11 display ID (:0, :1, etc) */
    const int monitorID;
    const vec2i size, position;
    const bool stereo;
    const bool doFullScreen;
    const std::string title;
  };
    
} // ::dw2
