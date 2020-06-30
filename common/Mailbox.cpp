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

#include "Mailbox.h"

namespace dw2 {

  /*! put a new message into the mailbox - if it matches the current
    frame put it into the active queue and notify any potential
    waiters; otherwise defer the message by putting it into the
    futureFrameMessages vector */
  void Mailbox::put(Mailbox::Message::SP newMessage)
  {
    std::lock_guard<std::mutex> lock(mutex);
    locked_put(newMessage);
  }
  /*! put a new message into the mailbox - if it matches the current
    frame put it into the active queue and notify any potential
    waiters; otherwise defer the message by putting it into the
    futureFrameMessages vector */
  void Mailbox::locked_put(Mailbox::Message::SP newMessage)
  {
    messages.push_back(newMessage);
    newMessageAvailable.notify_all();
  }

  /*! get next message that's ready for processing. if no messages
    are available for the current frame this will block until
    either a new message arrives, or a new frame is started and
    messages for that new frame arrive */
  Mailbox::Message::SP Mailbox::get()
  {
    std::unique_lock<std::mutex> lock(mutex);
    while (messages.empty())
      newMessageAvailable.wait(lock);

    auto ret = messages.front();
    messages.pop_front();
    //    std::cout << "found message in queue, size = " << ret->size() << "\n";
    return ret;
  }


  
  /*! start a new frame, and rec-onsider al future frame messages
    that we have already received */
  void TimeStampedMailbox::startNewFrame(int frameID)
  {
    // ------------------------------------------------------------------
    // first, retreive the currently delayed messages (so we can
    // then 're-send' them), and increase frame ID.
    // ------------------------------------------------------------------
    
    std::lock_guard<std::mutex> lock(mutex);
    currentFrameID   = frameID;
    std::vector<Mailbox::Message::SP> deferredMessages = retrieveDeferredMessages();
    //std::cout << "#mailbox: starting new frame " << frameID
	//<< ", reactivating " << deferredMessages.size() << " messages" << "\n";
  
    // ------------------------------------------------------------------
    // now that the new frame is active, re-put all the previously
    // deferred messages
    // ------------------------------------------------------------------
    for (auto message : deferredMessages) 
      locked_put(message);
  }


  /*! the actual core of the put() method, assuming the mutex is
    already locked */
  void TimeStampedMailbox::locked_put(Message::SP newMessage)
  {
    assert(newMessage);
    TileStampedMessageHeader *header = (TileStampedMessageHeader*)newMessage->data();
    
    if (header->frameID < currentFrameID) {
      /*! this is VERY unlikely, but in theory a tile _could_ contain
          all "inactive" pixels (eg, fall exactly into a bezel), in
          which case it _is_ possible that the previous frame was
          marked complete (because all active pixels were received),
          and a new frame already started. _very_ unlikely, but
          possible, so let's just accept and drop those - since
          they're guaranteed inactive (else the prev frame could never
          have been completed) it's safe to drop those ... */
      //std::cout << "Yay! Found a stale tile ... how's the chance of _that_!?" << "\n";
      return;
    }
      
    if (header->frameID == currentFrameID) {
      Mailbox::locked_put(newMessage);
    } else {
      // std::cout << "delaying " << header->frameID << " != " << currentFrameID << "\n";
      futureFrameMessages.push_back(newMessage);
    }
  }

  /*! put a new message into the mailbox - if it matches the current
    frame put it into the active queue and notify any potential
    waiters; otherwise defer the message by putting it into the
    futureFrameMessages vector */
  void TimeStampedMailbox::put(Mailbox::Message::SP newMessage)
  {
    std::lock_guard<std::mutex> lock(mutex);
    locked_put(newMessage);
  }

  /*! retreive - and empty - the vector of future-frame messages - may
      ONLY be called with the mutex already locked! */
  std::vector<Mailbox::Message::SP> TimeStampedMailbox::retrieveDeferredMessages()
  {
    std::vector<Message::SP> rv = futureFrameMessages;
    futureFrameMessages.clear();
    return rv;
  }
}
