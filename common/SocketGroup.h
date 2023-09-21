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

// #include "mpi_util.h"
#include "Socket.h"
#include "Mailbox.h"
// std
#include <vector>
#include <deque>

namespace dw2 {

  /*! a entire group of sockets to N remote nodes */
  struct SocketGroup {
    typedef std::shared_ptr<SocketGroup> SP;

    struct Remote {
      typedef std::shared_ptr<Remote> SP;
      //Mailbox::SP    outbox;
      Mailbox::SP    inbox;
      sock::socket_t socket;
    };
	std::thread    sendThread;
	std::thread    recvThread;
	Mailbox::SP    outbox;

    /*! create a new socket group that connects to the given node(s)
        using the provided magic cookie */
    SocketGroup(const size_t magic, const int numPeers,
                const std::vector<std::pair<std::string,int>> remotes);

    /*! create a new listening socket group that will accept only
        incoming connections with the given magic cookie */
    SocketGroup(const size_t magic, Mailbox::SP inbox,
               const size_t listenPort = 0);

    /*! send given message to given remote rank */
    void sendTo(std::vector<int> remoteRanks, Mailbox::Message::SP message);

    /*! broadcast message to all remotes */
    void broadcast(Mailbox::Message::SP message);

    // /*! get a message (from _any_ rank); blocks until one is available */
    // Mailbox::Message::SP get() { return inbox->get(); }

    /*! waits until _all_ remotes are connected */
    void waitForRemotesToConnect();

    int getPort() const { return portWeAreListeningOn; }

    std::vector<Remote::SP> remotes;
    
  private:
    void sendThreadFct();
    void recvThreadFct();

    /*! mutex for initial sync in waitForRemotesToConnect() */
    std::mutex              mutex;
    /*! flags true once we have all remotes connected */
    std::condition_variable allRemotesConnected;

    /*! the thread we use to accept incoming connections - only for
        'listening' socket groups */
    std::thread accepterThread;
    void accepterThreadFct();
    
    /*! number of remote nodes we are expecting. initially -1 because
        we simply don't know, later the incomig connections tell us
        how many peers there are ... */
    int numRemotesExpected = -1;

    int portWeAreListeningOn { -1 };
  };
  
}
