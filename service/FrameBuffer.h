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

// ospray
#include "../common/vec.h"
#include "../common/mpi_util.h"
// std
#include <memory>
#include <vector>

namespace dw2 {
  
  /*! a (stereo-capable) frame buffer */
  struct FrameBuffer {
    typedef std::shared_ptr<FrameBuffer> SP;
    
    FrameBuffer(const vec2i &size, bool stereo=false)
      : size(size),
        leftEyePixels(size.x*size.y),
        rightEyePixels(stereo?size.x*size.y:0)
    {}
    
    const vec2i size;
    std::vector<uint32_t> leftEyePixels;
    std::vector<uint32_t> rightEyePixels;
  };

} // ::dw2
