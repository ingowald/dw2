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

#include "Socket.h"

#include <vector>
#include <deque>
#include <stdint.h>
#include <thread>
#include <condition_variable>

namespace dw2 {

  /*! a mailbox that tracks a set of timestamped messages. For any
    given frame it will allow only messages with current time stamp
    to get through */
  struct Mailbox {
    typedef std::shared_ptr<Mailbox> SP;
    
    struct Message : public std::vector<uint8_t>
    {
      typedef std::shared_ptr<Message> SP;

      /*! frame ID that this message pertains to. shold always be
          newer or equal to the frame ID that the mailbox expects; if
          it matches exactly it is passed on to get() callers; if it
          is for a future frame, it will be detained until the next
          frmae gets activated */
      // int frameID;
      // int eye;

      // const box2i region;
      // Is this used?
      size_t outSize;
	  std::vector<int> toRank;
    };

    /*! put a new message into the mailbox, and notify whoever may be
        waiting for one */
    virtual void put(Message::SP newMessage);
    /*! the actual core of the put() method, assuming the mutex is
        already locked */
    void locked_put(Message::SP newMessage);

    /*! get next message that's ready for processing; y, wait until
        one arrives */
    virtual Message::SP get();
    
  protected:
    std::deque<Message::SP>  messages;
    std::mutex               mutex;
    std::condition_variable  newMessageAvailable;
  };


  /*! a mailbox whose get() function only let's through messages of
      the current time stamp, and delays all others until a respective
      frame with this time stamp has been started. This inherits from
      a regular mailbox, and uses that mailbox's get() function and
      'messages' queue, which means that this messages() queue is a
      queue that contains only messages with current frame ID; we
      realize this by overriding put() to delay messages with future
      frame IDs, and re-putting deferred messages when a new frame
      gets started */
  struct TimeStampedMailbox : public Mailbox {
    typedef std::shared_ptr<TimeStampedMailbox> SP;
    
    struct TileStampedMessageHeader {
      int frameID;
    };
    
    /*! start a new frame, and rec-onsider al future frame messages
        that we have already received */
    void startNewFrame(int frameID);

    /*! put a new message into the mailbox - if it matches the current
        frame put it into the active queue and notify any potential
        waiters; otherwise defer the message by putting it into the
        futureFrameMessages vector */
    virtual void put(Message::SP newMessage);

  private:
    /*! the actual core of the put() method, assuming the mutex is
        already locked */
    void locked_put(Message::SP newMessage);
    std::vector<Message::SP> futureFrameMessages;
    
    int                      currentFrameID = -1;
    
    /*! retreive - and empty - the vector of future-frame messages */
    std::vector<Message::SP> retrieveDeferredMessages();
  };
  
} // ::dw2
