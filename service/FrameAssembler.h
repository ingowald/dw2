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

#include "../common/Mailbox.h"
#include "FrameBuffer.h"

namespace dw2 {

  struct FrameToBe : public FrameBuffer {
    typedef std::shared_ptr<FrameToBe> SP;

    typedef enum { FRAME_NOW_COMPLETED, FRAME_NOT_YET_DONE } MarkCompletionResult;

    FrameToBe(size_t frameID, const vec2i &size, bool stereo)
      : FrameBuffer(size,stereo),
        frameID(frameID),
        numPixelsExpected(size.x*size.y)
    {}

    MarkCompletionResult markPixelsCompleted(size_t numNewPixels);
    
    const size_t frameID;
  private:
    friend class FrameAssembler;
    
    std::mutex   mutex;
    
    /*! total number of pixels already written this frame */
    size_t       numPixelsCompleted = 0;
    
    /*! total number of pixels we have to write this frame until we
      have a full frame buffer */
    const size_t numPixelsExpected = 0;
  };


  /*! class that is responsible for assembling ONE frame at a time. */
  struct FrameAssembler {
    typedef std::shared_ptr<FrameAssembler> SP;
    
    /*! construct a new assembler, and start the assembly process */
    FrameAssembler(/*! the inbox that will contain 'setTile' messages
                       (and nothing else) */
                   TimeStampedMailbox::SP inbox,
                   const box2i &myRegion,
                   bool         stereo=false,
                   int numThreads = 4);

    /*! get next fully-assembled frame; will wait until one is
        available. */
    FrameBuffer::SP collectAssembledFrame();
    
  private:
    /*! function that runs the actual assembler threads */
    void assemblerThreadFunction();
    
    /*! create a new frame buffer to be, and return the old one. may
        only get called by a thread that has acquired the mutex */
    void startOnNewFrame(int newFrameID);

    /*! gets called by the assembler thread that wrote the last pixels
        that completed a frame */
    void finishFrame(FrameToBe::SP finishedFrame);
    
    std::mutex mutex;

    FrameToBe::SP getCurrentFrame() {
      std::lock_guard<std::mutex> lock(mutex);
      return _currentFrame;
    }

    void setCurrentFrame(FrameToBe::SP newFrame) {
      std::lock_guard<std::mutex> lock(mutex);
      _currentFrame = newFrame;
    }
    
    /*! the frame we are currently assembling */
    FrameToBe::SP               _currentFrame;
    
    /*! list of frames that have been assembled, but not yet collected
        yet. "usually" somebody should already be waiting for a frame,
        but this is the easiest/cleanest way of decoupling frame
        assembly from frame consumption (ie, display) */
    std::deque<FrameToBe::SP>   finishedFrames;
    
    std::condition_variable     finishedFramesAvailable;

    /*! vector of the assembly threads we use to assemble frames */
    std::vector<std::thread>    assemblerThreads;
    TimeStampedMailbox::SP      inbox;
    
    /*! region that this assembler is resonsible for - we need to know
        this to know where incoming tiles go to in the frame buffer,
        and how many pixels we are expecting */
    const box2i                 myRegion;
    const bool                  stereo;
  };
  
} // ::dw2
