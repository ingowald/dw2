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

#include "Dispatcher.h"
#include "../common/CompressedTile.h"

namespace dw2 {

  /*! construct a new dispatcher, and start the dispatching process */
  Dispatcher::Dispatcher(/*! the inbox that will contain 'setTile' messages
                           (and nothing else) */
                         Mailbox::SP     inbox,
                         const std::vector<box2i> &regionOfRank,
                         mpi::Comm                 displayComm)
    : inbox(inbox),
      regionOfRank(regionOfRank),
      displayComm(displayComm)
  {
    dispatchThread = std::thread([this](){this->dispatchThreadFunction();});
  }
  
  /* performs the actual work - take tiles off the inbox, and dispatch
     them to whoeve rneeds them */
  void Dispatcher::dispatchThreadFunction()
  {
    if (displayComm.rank() == 0) {
      // ------------------------------------------------------------------
      // head node: dispatch tiles ...
      // ------------------------------------------------------------------
      while (1) {
        Mailbox::Message::SP message = inbox->get();
        const TileMessageDataHeader *header = (const TileMessageDataHeader *)message->data();
        const box2i region = header->region;
	//std::cout << "Head node dispatcher ... " << std::endl;
        for (int remoteID=0;remoteID<regionOfRank.size();remoteID++) {
          if (region.overlaps(regionOfRank[remoteID]))
            // displayComm->forwardTo(remoteID,message);
            MPI_CALL(Send(message->data(),message->size(),
                          MPI_BYTE,remoteID,0,displayComm.comm));
        }
      }
    } else {
      // ------------------------------------------------------------------
      // display node - receive tiles
      // ------------------------------------------------------------------
      while (1) {
        MPI_Status status;
        MPI_CALL(Probe(0,0,displayComm.comm,&status));
        int numBytes;
        MPI_CALL(Get_count(&status,MPI_BYTE,&numBytes));
        Mailbox::Message::SP message = std::make_shared<Mailbox::Message>();
        message->resize(numBytes);
        MPI_CALL(Recv(message->data(),numBytes,MPI_BYTE,0,0,
                      displayComm.comm,MPI_STATUS_IGNORE));
        inbox->put(message);
      }
    }
  }


  // void DisplayComm::forwardTo(int displayID, Mailbox::Message::SP message)
  // {
  //   const int tag  = uniqueTag++;
  //   const int dest = displayID+1;
  //   MPI_CALL(Send(message->data(),message->size(),MPI_BYTE,dest,tag,comm.comm));
  // }
  
} // ::dw2
