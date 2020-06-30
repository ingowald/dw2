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

#include "FrameAssembler.h"
#include "InfoServer.h"
#include "Dispatcher.h"
#include "../common/Mailbox.h"
#include "../common/SocketGroup.h"

namespace dw2 {
  
  // #define DW_STEREO 1
  // #define DW_HAVE_HEAD_NODE 2
  
  typedef void (*DisplayCallback)(FrameBuffer::SP newFB,
                                  void *objects);

// #endif
  
  // void startDisplayWallService(const mpi::Comm comm,
  //                              const WallConfig &wallConfig,
  //                              bool hasHeadNode,
  //                              DisplayCallback displayCallback,
  //                              void *objectForCallback,
  //                              int desiredInfoPortNum);

  struct Server
  {
    struct Config {

      box2i regionOfDisplay(int displayNo) const
      {
        if (displayNo < 0)
          return box2i({-1,-1},{-1,-1});
        
        const vec2i displayID(displayNo % numDisplays.x,
                              displayNo / numDisplays.x);
        vec2i lower = displayID * (windowSize + bezelWidth);
        vec2i upper = lower + windowSize;
        box2i region(lower,upper);
        return region;
      }
      
      bool  useHeadNode           { false };
      bool  hasControlWindow      { false };
      bool  doStereo              { false };
      bool  doFullScreen          { false };
      vec2i bezelWidth            {   0,   0 };
      vec2i windowSize            { 320, 240 };
      vec2i numDisplays           {   0,   0 };
      vec2i controlWindowPosition {   0,   0 };
      vec2i controlWindowSize     { 512, 512 };
      int   desiredInfoPortNum    { 2903 };
      int   maxFramesInFlight     { 1 };
	  int   headNodePort          { 0 };
    };

    Server(const Config &config);


    // ------------------------------------------------------------------
    // the interface for 'main()' to interact with this server:
    // ------------------------------------------------------------------

    /*! wait for the next frame to be fully assembled; this will block
        the caller until a frame is availale (and usually, until _all_
        ranks have reiceived their frame!) */
    FrameBuffer::SP waitForNextAssembledFrame();

  private:
    /*! gets called one per frame by *every* rank of the server (ie,
      _inluding_ the head node if one exists). As such, should never
      be called by main() directly, as main() will never return from
      Server() constructor on head node ... */
    void syncOnFrameReceived();
    
    /*! the mutex we can use as a monitor */
    std::mutex mutex;
    
    /*! the frame assembler - guess what - assembles the currnet
        frame; this is where the main functoin can get() the next
        ready frame */
    FrameAssembler::SP frameAssembler;

    /*! the inbox that the service's frame assembler will pull from;
        we need to track that explicitly here because this inbox is
        time stamped and thus has to be 'startNewFrame'd once per
        frame */
    TimeStampedMailbox::SP inbox;
    
    TimeStampedMailbox::SP ControlWindowinbox;
    
    /*! the dispatcher we use to dispatch tiles if we use a head
        node. will be null if in non-head node mode */
    Dispatcher::SP dispatcher;
    
    /*! a dedicated mpi communicator (dup'ed from world) that we can
        use to barrier within the server */
    mpi::Comm          syncOnFrameReceivedComm;
    mpi::Comm          world;

    /*! group of client connection sockets - when using a head node
        this will be empty on all display ranks other than head
        node */
    SocketGroup::SP clients;

    SocketGroup::SP m_client;

    /*! creates the service info to be served on the info server */
    ServiceInfo::SP createServiceInfo(size_t magic);
    // ControlWindowImageInfo::SP createControlWindowImageInfo(size_t magic);
    
    /*! the info server that, guess what, serves the service info to
        requesters. will be non-null ONLY on rank 0 (either head node
        or first display) */
    InfoServer::SP infoServer;
    // ControlWindowServer::SP controlWindowServer;

    
    const Config &config;
  };
    
} // ::dw2
