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

//#include "include/dw2.h"
#include "dw2_client.h"
#include "../common/ServiceInfo.h"
#include "../common/SocketGroup.h"
#include "../common/CompressedTile.h"

namespace dw2 {

  int dbg_rank = -1;

  int g_frameID = 0;

  struct Client {
    typedef std::shared_ptr<Client> SP;

    Client(const char *hostName, int port, int numPeers, int numThreads=16);
    
    void compressorThreadFunc();

    void put(PlainTile::SP tile) {
      std::lock_guard<std::mutex> lock(mutex);
      tilesToSend.push_back(tile);
      newWorkAvailable.notify_one();
    }
    
    std::mutex                mutex;
    std::condition_variable   newWorkAvailable;
    std::deque<PlainTile::SP> tilesToSend;
    std::vector<std::thread>  compressorThreads;
    SocketGroup::SP serviceSockets;
    SocketGroup::SP controlWindowServiceSocket;
    ServiceInfo::SP serviceInfo;
    // ControlWindowImageInfo::SP controlWindowImageInfo;

    // compression ratio
    std::vector<float> compressRatio;
  };


  void Client::compressorThreadFunc()
  {
    TileEncoder::SP encoder = TileEncoder::create();
    
    while (1) {
      PlainTile::SP tile;
      {
        std::unique_lock<std::mutex> lock(mutex);
        while (tilesToSend.empty())
          newWorkAvailable.wait(lock);
        tile = tilesToSend.front();
        tilesToSend.pop_front();
      }
      Mailbox::Message::SP tileMessage = encoder->encode(*tile);
      // std::cout << "tile size after compression : " << tileMessage ->outSize << "\n";
      // compressRatio.push_back((float)tileMessage ->outSize);
      // compressRatio.push_back(1 - ((float)tileMessage ->outSize / (tile->size().x * tile->size().y * 4)));
      // ------------------------------------------------------------------
      // now put this message into every remote that needs it.
      // ------------------------------------------------------------------
      std::vector<int> toRanks;
      for (int remoteID = 0; remoteID < serviceInfo->nodes.size(); remoteID++) {
        if (serviceInfo->nodes[remoteID].region.overlaps(tile->region)) {
          //serviceSockets->remotes[remoteID]->outbox->put(tileMessage);
          toRanks.push_back(remoteID);
        }
      }
      serviceSockets->sendTo(toRanks, tileMessage);
    }
  }
  

  
  Client::Client(const char *hostName, int port, int numPeers, int numThreads)
    : compressorThreads(numThreads)
  {
    // std::cout << "num threads : " << numThreads << "\n";
    for (auto &ct : compressorThreads)
      ct = std::thread([this](){this->compressorThreadFunc();});

    serviceInfo = ServiceInfo::getInfo(hostName, port);
    assert(serviceInfo);
    std::cout << "dw2.clent: receive service info (Client::Client) " << "\n";

    // controlWindowImageInfo = ControlWindowImageInfo::getInfo(hostName, port);
    // assert(controlWindowImageInfo);
    
    // ------------------------------------------------------------------
    // create a new socket group that connects to the given node(s)
    // using the provided magic cookie 
    // ------------------------------------------------------------------

    std::vector<std::pair<std::string,int>> remotes;
    for (auto &remote : serviceInfo->nodes) {
      remotes.push_back(std::pair<std::string,int>(remote.hostName,remote.port));
	  std::cout << "dw2.client: connecting to remote: " << remote.hostName << ":" << remote.port << "\n";
	}
    // std::cout << "#dw2.client(" << dbg_rank << "): starting socket group to display service" << "\n";
    serviceSockets = std::make_shared<SocketGroup>(serviceInfo->magic,numPeers,remotes);
    // std::cout << "#dw2.client(" << dbg_rank << "): connection established... waiting for handshake" << "\n";
    // ------------------------------------------------------------------
    // and do 'soft barrier' by reading one 'welcome' message from
    // each remote; it's the job of the remotes to send a message to
    // each connector upon first connection. Note the content of the
    // message doesn't matter at all, it gets discarded. Since each
    // render node waits for an input from each service node this will
    // get stuck until all render nodes have connected to all service
    // nodes.
    // ------------------------------------------------------------------
    // int numReceived = 0;
    for (auto &remote : serviceSockets->remotes) {
      Mailbox::Message::SP token = remote->inbox->get();
      //numReceived++;
      //std::cout << "#dw2.client(" << dbg_rank << "): got back token #" << numReceived 
      //          << "/" << serviceSockets->remotes.size() << " for frame " << *(int*)token->data() << "\n";
    }

    // // ------------------------------------------------------------------
    // // done ...
    // // ------------------------------------------------------------------
    // std::cout << "#dw2.client(" << dbg_rank << "): handshake completed established" << "\n";   

    //   std::vector<std::pair<std::string,int>> remotes;
    //   remotes.push_back(std::pair<std::string,int>(controlWindowImageInfo->node.hostName,controlWindowImageInfo->node.port));
    //   std::cout << "#dw2.client(" << dbg_rank << "): starting control window socket group to display service" << "\n";
    //   controlWindowServiceSocket = std::make_shared<SocketGroup>(controlWindowImageInfo->magic,numPeers,remotes);
    //   std::cout << "#dw2.client(" << dbg_rank << "): control window connection established... waiting for handshake" << "\n";

    //   for (auto &remote : controlWindowServiceSocket->remotes){
    //     Mailbox::Message::SP token = remote->inbox->get();
    //     //numReceived++;
    //     // std::cout << "#dw2.client(" << dbg_rank << "): got back token #" << numReceived 
    //     //          << "/" << serviceSockets->remotes.size() << " for frame " << *(int*)token->data() << "\n";
    //   }
    //   std::cout << "#dw2.client(" << dbg_rank << "): control window handshake completed established" << "\n";   
  }
  
  Client::SP g_client;

  // Client::SP m_client;
  
  // SocketGroup::SP g_serviceSockets;
  // ServiceInfo::SP g_serviceInfo;

  /*! query information on that given address; can be done as often as
    desired before connecting, and does not require a connect. This
    allows an app to query the vailability and/or size of a wall
    before even creating the render nodes. */
  extern "C" dw2_rc dw2_query_info(dw2_info_t *info,
                                   const char *hostName,
                                   const int   port)
  {
    try {
      ServiceInfo::SP serviceInfo = ServiceInfo::getInfo(hostName, port);
      assert(serviceInfo);
      (vec2i&)info->totalPixelsInWall = serviceInfo->totalPixelsInWall;
      (vec2i&)info->numDisplays = serviceInfo->numDisplays;
      (int&)info ->hasControlWindow = serviceInfo ->hasControlWindow;
      (vec2i&)info ->controlWindowSize = serviceInfo ->controlWindowSize;
    } catch (const std::exception &e) {
        std::cout << e.what() << "\n";
        return DW2_ERROR;
    }
    return DW2_OK;
  }

  extern "C" dw2_rc dw2_connect(const char *hostName, int port, int numPeers)
  {
    // if(master){
    //   m_client = std::make_shared<Client>(hostName,port,numPeers, master, 1);
    // }else{
      g_client = std::make_shared<Client>(hostName,port,numPeers);
    // }
    return DW2_OK;
  }
  
  extern "C" void dw2_disconnect()
  {
    std::cout << "DISCONNECT" << "\n";
    std::cout << "DISCONNECT" << "\n";
    std::cout << "DISCONNECT" << "\n";
    std::cout << "DISCONNECT" << "\n";
    std::cout << "DISCONNECT" << "\n";
    _exit(0);
  }

  extern "C" void dw2_begin_frame()
  {
    //std::cout << "#dw2.client(" << dbg_rank << "): begin_frame" << "\n";
    
    // do another 'soft barrier' here, waiting for a ping from rank
    // 0. basically that measn that rank 0 has to send an int for each
    // frame it wants the render ndoes to send; if it wants to have
    // multiple frames in flight it has to 'prime' that pipeline by
    // initially sending several such tokens, then re-sending a new
    // one for every frame received.
    Mailbox::Message::SP token = g_client->serviceSockets->remotes[0]->inbox->get();
    //std::cout << "#dw2.client(" << dbg_rank << "): got back token for frame " << *(int*)token->data() << "\n";
  }
  
  extern "C" void dw2_end_frame()
  {
    // print compression info
    // float all = 0;
    // int num = g_client ->compressRatio.size();
    // for(int i = 0; i < g_client ->compressRatio.size(); i++){
    //   all+=g_client ->compressRatio[i];
    //   // g_client ->compressRatio.pop_back();
    // }
    // g_client ->compressRatio.clear();
    // for(int i = 0; i < g_client ->compressRatio.size(); i++){
    //   g_client ->compressRatio.pop_back();
    // }
    // std::cout << "#dw2.client("<< dbg_rank << ") : num is " << num << " all is " << all << " average save space " << (float)all / num * 100 << "%" << "\n";
    
    
    g_frameID++;
    //std::cout << "#dw2.client(" << dbg_rank << "): end_frame: next frame id is " << g_frameID << "\n";
  }

  /*! send a tile that goes to position (x0,y0) and has size (sizeX,
      sizeY), with given array of pixels. */
  extern "C" void dw2_send_rgba(int x0, int y0, int sizeX, int sizeY,
                     /*! pitch: the increment (in uints) between each
                         line and the next in the pixel[] array */
                     int pitch,
                     const uint32_t *pixel)
  {
#if 1
    PlainTile::SP tile = std::make_shared<PlainTile>();
    tile->alloc(box2i(vec2i(x0,y0),vec2i(x0+sizeX,y0+sizeY)),0);
    tile->frameID = g_frameID;
    uint32_t *out = (uint32_t *)tile->pixels.data();
    for (int iy=0;iy<sizeY;iy++) {
      const uint32_t *in = pixel + iy * pitch;
      for (int ix=0;ix<sizeX;ix++)
        *out++ = *in++;
    }
    g_client->put(tile);
#else
    // ------------------------------------------------------------------
    // copy the tile, and encode in a message
    // ------------------------------------------------------------------
    PlainTile tile;
    tile.alloc(box2i(vec2i(x0,y0),vec2i(x0+sizeX,y0+sizeY)),0);
    tile.frameID = g_frameID;
    uint32_t *out = (uint32_t *)tile.pixels.data();
    for (int iy=0;iy<sizeY;iy++) {
      const uint32_t *in = pixel + iy * pitch;
      for (int ix=0;ix<sizeX;ix++)
        *out++ = *in++;
    }
    Mailbox::Message::SP tileMessage = encoder->encode(tile);

    // ------------------------------------------------------------------
    // now put this message into every remote that needs it.
    // ------------------------------------------------------------------
    for (int remoteID = 0; remoteID < g_serviceInfo->nodes.size(); remoteID++)
      if (g_serviceInfo->nodes[remoteID].region.overlaps(tile.region)) {
        // std::cout << "sending " << tile.region << " to " << remoteID << " " << g_serviceInfo->nodes[remoteID].region << "\n";
        g_serviceSockets->remotes[remoteID]->outbox->put(tileMessage);
      }
#endif
  }

  
} // ::dw2
