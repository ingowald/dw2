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

#include "ServiceInfo.h"

namespace dw2 {

  /*! contact service at given host and port, request service, and
    return it back */
  ServiceInfo::SP ServiceInfo::getInfo(const std::string hostName, int port)
  {
    sock::socket_t socket = sock::connect(hostName.c_str(),port);
    if (!socket) return nullptr;

    // int server_version_major; read(socket,server_version_major);
    // int server_version_minor; read(socket,server_version_minor);
    // int server_version_patch; read(socket,server_version_patch);

#if 0
    if (server_version_major != DW2_VERSION_MAJOR ||
        server_version_minor != DW2_VERSION_MINOR ||
        server_version_patch != DW2_VERSION_PATCH) {
      std::cout << "#dw2.client: Warning: This is *different* from client version:" << "\n";
      std::cout << "#dw2.client: - service reported version is "
                << server_version_major << "."
                << server_version_minor << "."
                << server_version_patch
                << "\n";
      std::cout << "#dw2.client: - client library version is "
                << DW2_VERSION_MAJOR << "."
                << DW2_VERSION_MINOR << "."
                << DW2_VERSION_PATCH
                << "\n";
    } else 
      std::cout << "#dw2.client: service reports version "
                << DW2_VERSION_MAJOR << "." << DW2_VERSION_MINOR << "." << DW2_VERSION_PATCH
                << ".... perfect!" << "\n";
#endif
    
    size_t magic;
    read(socket,magic);
    
    ServiceInfo::SP info = std::make_shared<ServiceInfo>(magic);
    read(socket,info->totalPixelsInWall);
    read(socket,info->numDisplays);
    read(socket,info->stereo);
    read(socket, info ->hasControlWindow);
    read(socket, info ->controlWindowSize);
    
    int numNodes;
    read(socket,numNodes);
    info->nodes.resize(numNodes);
    
    for (auto &node : info->nodes) {
      read(socket,node.hostName);
      read(socket,node.port);
      read(socket,node.region);
    }

#if 1
    std::cout << "#dw2.client: service info reported the following displays: " << "\n";
    for (int i=0;i<info->nodes.size();i++)
      std::cout << "#dw2.client: - display #" << i << " " << info->nodes[i].region << "\n";
    // sleep(10);
#endif
    sock::close(socket);
    return info;
  }

  void ServiceInfo::writeTo(sock::socket_t socket)
  {
    // write(socket,(int)DW2_VERSION_MAJOR);
    // write(socket,(int)DW2_VERSION_MINOR);
    // write(socket,(int)DW2_VERSION_PATCH);
    write(socket,magic);

    write(socket,totalPixelsInWall);
    write(socket,numDisplays);
    write(socket,stereo);
    write(socket, hasControlWindow);
    write(socket, controlWindowSize);
    
    write(socket,(int)nodes.size());
    for (auto &node : nodes) {
      write(socket,node.hostName);
      write(socket,node.port);
      write(socket,node.region);
    }
  }

  // void ControlWindowImageInfo::writeTo(sock::socket_t socket)
  // {
  //   std::cout << "Iam HERE : magic " << magic << "\n";
  //   std::cout << " node hostname " << node.hostName << " port " << node.port << "\n";
  //   write(socket, magic);
  //   write(socket, node.hostName);
  //   write(socket, node.port);
  // }

  // ControlWindowImageInfo::SP ControlWindowImageInfo::getInfo(const std::string hostName, int port)
  // {
  //   sock::socket_t socket = sock::connect(hostName.c_str(),port);
  //   if (!socket){
  //     std::cerr << "Cannot connect to !!!!!!" << "\n";
  //     return nullptr;
  //   }
  //   size_t magic;
  //   read(socket,magic);
  //   std::cout << "Connnectttt to !!!!!!!!!!!" << " magic " << magic << "\n";
  //   ControlWindowImageInfo::SP info = std::make_shared<ControlWindowImageInfo>(magic);
  //   read(socket, info ->node.hostName);
  //   read(socket, info ->node.port);

  //   return info;
  // }
} // ::dw2
