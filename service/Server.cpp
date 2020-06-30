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

#include "Server.h"
#include "../common/CompressedTile.h"
#include "../common/mpi_util.h"
#ifndef WIN32
#include <sys/times.h>
#endif

namespace dw2 {

  /*! this is called by the APP to wait for the next frame */
  FrameBuffer::SP Server::waitForNextAssembledFrame()
  {
    // first, wait til _we_ have our frame ...
    FrameBuffer::SP frame = frameAssembler->collectAssembledFrame();

    // std::cout << "#server(" << world.rank() << "): $$$$$$$$$$$ start sync on frame " << "\n";
    syncOnFrameReceived();
    //std::cout << "#server(" << world.rank() << "): DONE sync on farme " << "\n";
    
    // now, if we're tank 0
    return frame;
  }

  /*! gets called one per frame by *every* rank of the server (ie,
      _inluding_ the head node if one exists). As such, should never
      be called by main() directly, as main() will never return from
      Server() constructor on head node ... */
  void Server::syncOnFrameReceived()
  {
    // std::cout << "#server(" << world.rank() << "):$$$$$$$$$$$$$$$$$$$$$$ sync on frame (IN) " << "\n";
    // barrier until everybody else got theirs, too...
    static int g_dbg_frameID = 0;
    //std::cout << "#server(" << world.rank() << "): entering barrier..." << g_dbg_frameID << "\n";
    
    syncOnFrameReceivedComm.barrier();
    //std::cout << "#server(" << world.rank() << "): leaving barrier..." << g_dbg_frameID++ << "\n";
    // std::cout << "#server(" << world.rank() << "):$$$$$$$$$$$$$$$$$$$$$$ sync on frame (OUT) - yay " << "\n";

    // now, if we're rank 0, send message back to clients that the
    // frame has been received.
    if (syncOnFrameReceivedComm.rank() == 0) {
      static int g_frameID = 0;
      int frameID = g_frameID++;
      Mailbox::Message::SP frameReceivedMessage = std::make_shared<Mailbox::Message>();
      frameReceivedMessage->resize(sizeof(frameID));
      memcpy((void *)frameReceivedMessage->data(),&frameID,sizeof(frameID));

      //std::cout << "############## server to clients: done frame " << frameID << "\n";
      assert(clients);
      clients->broadcast(frameReceivedMessage);

      //std::cout << "############## done broadcast " << frameID << "\n";
      assert(inbox);
      if (config.useHeadNode)
        // only do that on head node; if it's a regular diplay it
        // already does that in FrameAssembler::startOnNewFrame()
        inbox->startNewFrame(frameID+1);
      
      //std::cout << "############## server done restart of mailbox " << frameID << "\n";
    }
  }

  /*! creates the service info to be served on the info server */
  ServiceInfo::SP Server::createServiceInfo(size_t magic)
  {
    ServiceInfo::SP serviceInfo = std::make_shared<ServiceInfo>(magic);
    serviceInfo->numDisplays = config.numDisplays;
    serviceInfo->totalPixelsInWall
      = config.numDisplays * config.windowSize
      + (config.numDisplays - vec2i(1)) * config.bezelWidth;
    serviceInfo->stereo = config.doStereo;
    if(config.hasControlWindow){
      serviceInfo ->hasControlWindow = config.hasControlWindow;
      serviceInfo ->controlWindowSize = config.controlWindowSize;
    }
    
    if (config.useHeadNode) {
      // ==================================================================
      // WITH head node setup - only head node serves, and it does
      // serve entire display
      // ==================================================================
      if (world.rank() > 0) return nullptr;

      ServiceInfo::Node headNode;
      headNode.hostName = getHostName();
      headNode.port     = clients->getPort();
      headNode.region   = { vec2i(0,0), serviceInfo->totalPixelsInWall };
      serviceInfo->nodes.push_back(headNode);
      return serviceInfo;
      
    } else {
      // ==================================================================
      // NON head node setup - only rank 0 serves, but it has to serve
      // all display names...
      // ==================================================================
      
      std::vector<std::string> clientHostNames;
      std::vector<int>         clientPortNumbers;
      if (world.rank() == 0) {
        for (int peer=0;peer<world.size();peer++) {
          ServiceInfo::Node displayNode;
          if (peer == 0) {
            /* myself ... */
            displayNode.hostName = getHostName();
            displayNode.port     = clients->getPort();
            displayNode.region   = config.regionOfDisplay(0);
          } else {
            /* another display */
            displayNode.hostName = world.read<std::string>(peer);
            displayNode.port     = world.read<int>(peer);
            displayNode.region   = world.read<box2i>(peer);
          }
          serviceInfo->nodes.push_back(displayNode);
        }
        return serviceInfo;
      } else {
        world.write(0,getHostName());
        world.write(0,(int)clients->getPort());
        world.write(0,config.regionOfDisplay(world.rank()));
        // all other ranks can now return, only rank 0 will ever serve anything
        return nullptr;
      }
    }
  }
  
  // ControlWindowImageInfo::SP Server::createControlWindowImageInfo(size_t magic)
  // {
  //   ControlWindowImageInfo::SP controlWindowImageInfo = std::make_shared<ControlWindowImageInfo>(magic);
  //   ControlWindowImageInfo::Node headNode;
  //   headNode.hostName = getHostName();
  //   headNode.port     = clients->getPort();
  //   controlWindowImageInfo ->node.hostName = headNode.hostName;
  //   controlWindowImageInfo ->node.port = headNode.port;
  //   return controlWindowImageInfo;
  // }
  
  Server::Server(const Server::Config &config)
    : config(config),
      world(mpi::Comm(MPI_COMM_WORLD).dup())
  {
    // ------------------------------------------------------------------
    // create sync-barrier for end-of frame sync
    // ------------------------------------------------------------------
    syncOnFrameReceivedComm = world.dup();//mpi::Comm(MPI_COMM_WORLD).dup();

    size_t magic = times(nullptr);
    MPI_CALL(Bcast(&magic,1,MPI_LONG_INT,0,world.comm));
    
    // ------------------------------------------------------------------
    // create an inbox that peers or clients' messages can be placed
    // into
    // ------------------------------------------------------------------
    inbox = std::make_shared<TimeStampedMailbox>();
    // TimeStampedMailbox::SP inbox = std::make_shared<TimeStampedMailbox>();
    // this->timeStampedMailbox = inbox;
    
    // ------------------------------------------------------------------
    // init clients socketgroup - need to do that before serverinfo
    // because we need the port number(s)
    // ------------------------------------------------------------------
    const bool needClientConnections
      =  /* head node: */ config.useHeadNode && world.rank() == 0
      || /* all displays: */ !config.useHeadNode;
    if (needClientConnections)
      clients = std::make_shared<SocketGroup>(magic,inbox,config.headNodePort);
    world.barrier();

    // ------------------------------------------------------------------
    // gather and synchronize on display info
    // ------------------------------------------------------------------
    const int myDisplayID = config.useHeadNode ? world.rank()-1 : world.rank();
    const box2i myRegion  = config.regionOfDisplay(myDisplayID);
    // std::cout << "#dw2.server(" << world.rank() << "): rank #" << world.rank()
    //           << " serving region " << myRegion << "\n";
    world.barrier();
    
    // ------------------------------------------------------------------
    // start the info server
    // ------------------------------------------------------------------
    // create service info - HAS to be done collectively, so do _not_
    // move into a "rank()==0" section.
    ServiceInfo::SP serviceInfo
      = createServiceInfo(magic);
    if (serviceInfo)
      std::cout << "#dw2.server(" << world.rank() << "): service info created for total #pixels = "
                << serviceInfo->totalPixelsInWall
                << ", served on #nodes = "
                << serviceInfo->nodes.size() << "\n";

    if (world.rank() == 0) {
      infoServer = std::make_shared<InfoServer>(config.desiredInfoPortNum,serviceInfo);
      std::cout << "#dw2.server(" << world.rank() << "): info server running on rank " << world.rank() << "\n";

      // if(config.hasControlWindow){
      //   size_t m = times(nullptr);
      //   ControlWindowinbox = std::make_shared<TimeStampedMailbox>();
      //   m_client = std::make_shared<SocketGroup>(m, ControlWindowinbox);
      //   ControlWindowImageInfo::SP controlWindowImageInfo = createControlWindowImageInfo(m);
      //   //! open another port for receiving frames for the control window
      //   int p = 8443;
      //   controlWindowServer = std::make_shared<ControlWindowServer>(p, controlWindowImageInfo);
      // }
    }
     world.barrier();
    

    // ------------------------------------------------------------------
    // create frame assemblers
    // ------------------------------------------------------------------
    bool willAssembleFrames = !config.useHeadNode || world.rank() != 0;
    if (willAssembleFrames) {
      std::cout << "#dw2.server(" << world.rank() << "): creating frame assembler on rank " << world.rank() << "\n";
      frameAssembler = std::make_shared<FrameAssembler>(inbox,myRegion);
      std::cout << "#dw2.server(" << world.rank() << "): created frame assembler on rank #"
                << world.rank() << " region " << myRegion << "\n";
    }
    world.barrier();

    // ------------------------------------------------------------------
    // hook up dispatcher, if required
    // ------------------------------------------------------------------
    if (config.useHeadNode) {
      std::vector<box2i> regionOfRank;
      for (int rank=0;rank<world.size();rank++)
        regionOfRank.push_back(config.regionOfDisplay(rank-1));
      dispatcher = std::make_shared<Dispatcher>(inbox,regionOfRank,world);
      std::cout << "#dw2.server(" << world.rank() << "): dispatcher started..." << "\n";
    }
    world.barrier();
    
    // ------------------------------------------------------------------
    // wait for incoming connections
    // ------------------------------------------------------------------
    if (clients) {
      std::cout << "#dw2.server(" << world.rank() << "): rank " << world.rank()
                << " waiting for remote connections..." << "\n";
      clients->waitForRemotesToConnect();
      std::cout << "#dw2.server(" << world.rank() << "): all clients connected, send first handshake..." << "\n";
      Mailbox::Message::SP handShakeMessage = std::make_shared<Mailbox::Message>();
      handShakeMessage->resize(13);
      clients->broadcast(handShakeMessage);
    }
    world.barrier();
    // if(m_client){
    //   std::cout << "#dw2.server(" << world.rank() << "): rank " << world.rank()
    //             << " waiting for control window remote connections..." << "\n";
    //   m_client->waitForRemotesToConnect();
    //   std::cout << "#dw2.server(" << world.rank() << "): all clients control window connected, send first handshake..." << "\n";
    //   Mailbox::Message::SP handShakeMessage = std::make_shared<Mailbox::Message>();
    //   handShakeMessage->resize(13);
    //   std::cout << "remotes size " << m_client ->remotes.size() << "\n";
    //   for (auto &remote : m_client->remotes){
    //     // std::cout << "remote hostname " << remote.hostName << std::endl;
    //     remote->outbox->put(handShakeMessage);
    //   }
    // }
        
    // world.barrier();

    
    // ------------------------------------------------------------------
    // send initial tokens (for how many frames are allowed to be in
    // flight)
    // ------------------------------------------------------------------
    assert(config.maxFramesInFlight > 0);
    if (world.rank() == 0) {
      for (int i=0;i<config.maxFramesInFlight;i++) {
        Mailbox::Message::SP message = std::make_shared<Mailbox::Message>();
        // content doesn't actually matter, it's just a dummy token, anyway. 
		// TODO: Why not just send an int?
        message->resize(i+1);
        clients->broadcast(message);
        //for (auto &remote : clients->remotes)
         // remote->outbox->put(message);
      }
    }

    // ------------------------------------------------------------------
    // start accepting tiles ...
    // ------------------------------------------------------------------
    inbox->startNewFrame(0);
    
    // ------------------------------------------------------------------
    // if head node, run the barrier thread (ie, barrier with displays
    // at end of frame, then send token back to clients)
    // ------------------------------------------------------------------
    if (config.useHeadNode && world.rank() == 0) {
      while (1) {
	      double t_start = getCurrentTime();
        //std::cout << "#dw2.head: waiting for displays to mark end of frame" << "\n";
        syncOnFrameReceived();
        //std::cout << "#dw2.head: done starting new frame!" << "\n";
	double t_end = getCurrentTime();
	std::cout << "dw2.head: frame rate: " << 1.f / (t_end - t_start) << " fps" << std::endl;
      }
      /* THIS WILL NEVER RETURN (which is OK) */
    }
    
    // ------------------------------------------------------------------
    // done ... return to app.
    // ------------------------------------------------------------------
    std::cout << "#dw2.server: rank " << world.rank()
              << " returning back to main() to handle displays..." << "\n";
    return;
  }

  
} // ::dw2
