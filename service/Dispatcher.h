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
#include "../common/Mailbox.h"
#include "../common/ServiceInfo.h"
// std
#include <atomic>

namespace dw2 {

  // /*! a communicator that sends tiles on to displays ... */
  // struct DisplayComm
  // {
  //   typedef std::shared_ptr<DisplayComm> SP;
    
  //   void forwardTo(int displayID, Mailbox::Message::SP message);
  // private:
  //   mpi::Comm comm;
  //   std::atomic<int> uniqueTag;
  // };
    
  /*! dispatches tiles to the actual display nodes. This class doesn't
      do anything other than taking messages off the inbox, and
      dispatching messages to the appropriate clients - no encoding or
      decoding of tiles, no providing feedback to the render nodes
      (that's somebody else's job), nothing ... just take a tile, and
      send it on to anybody that needs it. */
  struct Dispatcher {
    typedef std::shared_ptr<Dispatcher> SP;
    
    /*! construct a new dispatcher, and start the dispatching process */
    Dispatcher(/*! the inbox that will contain 'setTile' messages
                 (and nothing else) */
               Mailbox::SP               inbox,
               const std::vector<box2i> &regionOfRank,
               mpi::Comm                 displayComm);

  private:
    /* performs the actual work - take tiles off the inbox, and
       dispatch them to whoeve rneeds them */
    void dispatchThreadFunction();
    
    /*! the thread we use to perform the dispatching */
    std::thread              dispatchThread;

    /*! the inbox we're reading from */
    Mailbox::SP              inbox;

    const std::vector<box2i> regionOfRank;

    /*! the communicator we use to send tiles to displays */
    mpi::Comm                displayComm;
  };
  
}
