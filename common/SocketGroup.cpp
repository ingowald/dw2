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

#include "SocketGroup.h"

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#else 
#include <sys/socket.h>
#include <poll.h>
#endif

// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netinet/tcp.h>
// #include <netdb.h> 

namespace dw2 {

  /*! create a new socket group that connects to the given node(s)
    using the provided magic cookie */
  SocketGroup::SocketGroup(const size_t magic, const int numPeers,
                           const std::vector<std::pair<std::string,int>> remoteURLs)
    : numRemotesExpected(remoteURLs.size())
  {
    outbox = std::make_shared<Mailbox>();
    for (auto &url : remoteURLs) {
      Remote::SP remote = std::make_shared<Remote>();
      remote->socket = sock::connect(url.first.c_str(),url.second);
      // for the clients, each remote gets their own mailbox
      remote->inbox  = std::make_shared<Mailbox>();
      //remote->outbox = std::make_shared<Mailbox>();
      // PING; 
      PRINT(magic); PRINT(numPeers);
      write(remote->socket,(size_t)magic);
      write(remote->socket,(int)numPeers);
      sock::flush(remote->socket);
      remotes.push_back(remote);
    }
    sendThread = std::thread([&](){sendThreadFct();});
    recvThread = std::thread([&](){recvThreadFct();});
    std::cout << "#dw.src: all remotes connected" << "\n";
  }

  void SocketGroup::sendThreadFct()
  {
    while (1) {
      //assert(remote);
      //assert(remote->outbox);
      Mailbox::Message::SP message = outbox->get();
      int sizeData = message->size();
      for (const auto &to : message->toRank) {
        // std::cout << "sending msg of size " << sizeData << ", to rank "
          // << to << "\n" << std::flush;
        write(remotes[to]->socket,&sizeData,sizeof(sizeData));
        write(remotes[to]->socket,message->data(),sizeData);
        sock::flush(remotes[to]->socket);
        // std::cout << "wrote message to socket ..." << "\n";
      }
    }
  }
  
  void SocketGroup::recvThreadFct()
  {
    std::vector<pollfd> pollFds;
    for (const auto &r : remotes) {
      pollfd p;
      p.fd = getFileDescriptor(r->socket);
      p.events = POLLIN;
      pollFds.push_back(p);
    }

    while (1) {
      int nready = 0;
      do {
#ifdef _WIN32
        nready = WSAPoll(pollFds.data(), pollFds.size(), -1);
#else
        nready = poll(pollFds.data(), pollFds.size(), -1);
#endif
        if (nready == -1) {
          std::cout << "Error in select!\n" << std::flush;
          throw std::runtime_error("Error in select\n");
        }
      } while (nready == 0);

      for (size_t i = 0; i < remotes.size(); ++i) {
        const bool disconnected = pollFds[i].revents & POLLHUP;
        if (!(pollFds[i].revents & POLLIN)) {
          continue;
        }
        auto &r = remotes[i];
        int rfd = pollFds[i].fd;
        pollFds[i].revents = 0;

        int sizeData;
        int readn = ::recv(rfd, (char*)&sizeData, sizeof(sizeData), MSG_WAITALL);
        if (readn == 0 && disconnected) {
          std::cout << "Socket is disconnected and buffer empty, we should exit\n"
            << "TODO: Handle the closing of connection gracefully\n";
          std::exit(0);
        }

        Mailbox::Message::SP message = std::make_shared<Mailbox::Message>();
        message->resize(sizeData);
        ::recv(rfd, (char*)message->data(), message->size(), MSG_WAITALL);
        int msg = *reinterpret_cast<int*>(message->data());
        r->inbox->put(message);
      }
    }
  }

  /*! waits until _all_ remotes are connected */
  void SocketGroup::waitForRemotesToConnect()
  {
    std::unique_lock<std::mutex> lock(mutex);
    while (numRemotesExpected == -1 || remotes.size() != numRemotesExpected)
      allRemotesConnected.wait(lock);
  }
  
  /*! create a new listening socket group that will accept only
    incoming connections with the given magic cookie */
  SocketGroup::SocketGroup(const size_t myMagic, Mailbox::SP inbox,
                           const size_t listenPort)
  {
    sock::socket_t listener = sock::bind(listenPort);
    int port = sock::getPortOf(listener);

    std::cout << "#dw2.server listening for clients on "
		<< getHostName() << ":" << port << "\n";
    this->portWeAreListeningOn = port;
    outbox = std::make_shared<Mailbox>();
    
    accepterThread = std::thread([this,listener,myMagic,inbox](){
        while (1) {
          Remote::SP remote = std::make_shared<Remote>();
          remote->socket = sock::listen(listener);
          remote->inbox  = inbox;
          //remote->outbox = std::make_shared<Mailbox>();
          
          size_t remoteMagic;
          read(remote->socket,remoteMagic);
          if (remoteMagic != myMagic) {
            std::cout << "Wrong magic from remote ...." << "\n";
            sock::close(remote->socket);
            continue;
          }

          int remoteSize;
          read(remote->socket,remoteSize);
          if (numRemotesExpected == -1)
            numRemotesExpected = remoteSize;
          else
            assert(numRemotesExpected == remoteSize);

          remotes.push_back(remote);
          // std::cout << "#sockets. got remotes = " << remotes.size() << "\n";
          if (remotes.size() == numRemotesExpected) {
            allRemotesConnected.notify_all();
            break;
          }
        }
        sendThread = std::thread([&](){sendThreadFct();});
        recvThread = std::thread([&](){recvThreadFct();});
      });
  }


  /*! broadcast message to all remotes */
  void SocketGroup::broadcast(Mailbox::Message::SP message)
  {
    // PING; PRINT(message->size()); PRINT(remotes.size());
    // PRINT(mpi::Group(MPI_COMM_WORLD).rank());
    std::vector<int> remoteIds;
    for (int i=0;i<remotes.size();i++) {
      remoteIds.push_back(i);
    }
    sendTo(remoteIds, message);
  }
  
  /*! send given message to given remote rank */
  void SocketGroup::sendTo(std::vector<int> remoteRanks, Mailbox::Message::SP message)
  {
    // _probably_ don't need this lock here - remotes[] only changes
    // only during constructor (for outgoring) respectively before
    // 'waritForRemotesToConnect()' (for incoming), and sendTo sohld
    // never get called before the respectiv eone of those is complete
    // .... but just in case*/
    std::lock_guard<std::mutex> lock(mutex);
    assert(remotes.size() == numRemotesExpected);
    message->toRank = remoteRanks;
    //assert(remotes[remoteRank]->outbox);
    //remotes[remoteRank]->outbox->put(message);
    outbox->put(message);
  }
  
} // ::dw2
