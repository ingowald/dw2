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

#include "FrameAssembler.h"
#include "../common/CompressedTile.h"

namespace dw2 {

  FrameToBe::MarkCompletionResult FrameToBe::markPixelsCompleted(size_t numNewPixels)
  {
    std::lock_guard<std::mutex> lock(mutex);
    numPixelsCompleted += numNewPixels;
   // std::cout << "#assembler: assembled " << numNewPixels << " pixels, now have " << numPixelsCompleted << "/" << numPixelsExpected << "\n";
    return (numPixelsCompleted == numPixelsExpected)
      ? FRAME_NOW_COMPLETED
      : FRAME_NOT_YET_DONE;
  }

  /*! construct a new assembler, and start the assembly process */
  FrameAssembler::FrameAssembler(TimeStampedMailbox::SP inbox,
                                 const box2i &myRegion,
                                 bool stereo,
                                 int numThreads)
    : assemblerThreads(numThreads),
      inbox(inbox),
      myRegion(myRegion),
      stereo(stereo)
  {
    {
      std::lock_guard<std::mutex> lock(mutex);
    
      // start the threads ...
      for (auto &thread : assemblerThreads) 
        thread = std::thread([this]() { this->assemblerThreadFunction(); });
    }
    startOnNewFrame(0);
  }

  
  void FrameAssembler::assemblerThreadFunction()
  {
    TileDecoder::SP decoder = TileDecoder::create();
    
    while (1) {
      // ------------------------------------------------------------------
      // pull one tile from queue. Watch for race condition: inbox
      // should never get activated for a frame that hasn't been
      // started yet. *if* we do that, then we *can* guarantee the
      // following to be correct, because of the following argument:
      //
      // a) if we do get a message from get(), then this must be from
      // the *current* frame (else the mailbox wouldn't have let it
      // through)
      //
      // b) we also know the frame cannot be finished yet, else we'd
      // not have gotten a tile for it.
      //
      // c) we therefore know that 'currentFrame' is valid, because it
      // was created before the the message was ever let through to
      // us, and because it won't be overwritten until it has been
      // completed.
      // ------------------------------------------------------------------
      Mailbox::Message::SP message = inbox->get();

      FrameToBe::SP currentFrame = getCurrentFrame();
      
      PlainTile plainTile;
      decoder->decode(plainTile,message); 
    
      const box2i globalRegion  = plainTile.region;
      size_t numWritten = 0;
      const uint32_t *tilePixel = plainTile.pixels.data();
      uint32_t *localPixel
        = plainTile.eye==0
        ? currentFrame->leftEyePixels.data()
        : currentFrame->rightEyePixels.data();
      
      for (int iy=globalRegion.lower.y;iy<globalRegion.upper.y;iy++) {
              
        if (iy  < myRegion.lower.y) { continue; }
        if (iy >= myRegion.upper.y) { continue; }
              
        for (int ix=globalRegion.lower.x;ix<globalRegion.upper.x;ix++) {
          if (ix  < myRegion.lower.x) { continue; }
          if (ix >= myRegion.upper.x) { continue; }
                
          const vec2i globalCoord(ix,iy);
          const vec2i tileCoord = globalCoord-plainTile.region.lower;
          const vec2i localCoord = globalCoord-myRegion.lower;
          const int localPitch = myRegion.size().x;
          const int tilePitch  = plainTile.region.size().x;
          const int tileOfs = tileCoord.x + tilePitch * tileCoord.y;
          const int localOfs = localCoord.x + localPitch * localCoord.y;
          localPixel[localOfs] = tilePixel[tileOfs];
          ++numWritten;
        }
      }
      assert(numWritten > 0);
      if (currentFrame->markPixelsCompleted(numWritten) == FrameToBe::FRAME_NOW_COMPLETED) 
        finishFrame(currentFrame);
    }
  }


  /*! gets called by the assembler thread that wrote the last pixels
    that completed a frame */
  void FrameAssembler::finishFrame(FrameToBe::SP finishedFrame)
  {
    //std::cout << "#server(" << mpi::Comm(MPI_COMM_WORLD).rank() << "): frame completely assembled..." << "\n";
    
    // FrameToBe::SP finishedFrame = currentFrame;
    assert(finishedFrame);

    {
      std::lock_guard<std::mutex> lock(mutex);
      finishedFrames.push_back(finishedFrame);
      finishedFramesAvailable.notify_all();
    }
    
    startOnNewFrame(finishedFrame->frameID+1);
  }
    
  /*! create a new frame buffer to be, and return the old one. may
    only get called by a thread that has acquired the mutex */
  void FrameAssembler::startOnNewFrame(int newFrameID)
  {
   //std::cout << "#server(" << mpi::Comm(MPI_COMM_WORLD).rank() << "): starting on new frame " << newFrameID << "\n";
    /* must HAS to be locked when this is called, so don't lcok again */
    FrameToBe::SP newFrame = std::make_shared<FrameToBe>(newFrameID,
                                                         myRegion.size(),
                                                         stereo);
    setCurrentFrame(newFrame);
    inbox->startNewFrame(newFrameID);
  }

  FrameBuffer::SP FrameAssembler::collectAssembledFrame()
  {
    std::unique_lock<std::mutex> lock(mutex);
    while (finishedFrames.empty())
      finishedFramesAvailable.wait(lock);

    auto ret = finishedFrames.front();
    finishedFrames.pop_front();
    return ret;
  }
  
  
} // ::dw2

