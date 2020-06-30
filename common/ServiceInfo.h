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
#include "vec.h"
// std

namespace dw2 {

  /*! info structure that one can query from the display wall's info
    port. the app can query this info and then use this to have the
    client connect to the display wall */
  struct ServiceInfo {
    typedef std::shared_ptr<ServiceInfo> SP;
    
    /* constructor that initializes everything to default values */
    ServiceInfo(const size_t magic = 1) // random() is not defined on windows. Maybe switch to std::rand()?
      : totalPixelsInWall(-1,-1),
        controlWindowSize(0, 0),
        magic(magic)
    {
    }

    /*! contact service at given host and port, request service, and
        return it back */
    static ServiceInfo::SP getInfo(const std::string hostName, int port);
    
    /*! total pixels in the entire display wall, across all
      indvididual displays, and including bezels (future versios
      will allow to render to smaller resolutions, too - and have
      the clients upscale this - but for now the client(s) have to
      render at exactly this resolution */
    vec2i totalPixelsInWall;

    /*! number of displays, just for informational purposes */
    vec2i numDisplays;

    int hasControlWindow;
    vec2i controlWindowSize;
    
    /*! whether this runs in stereo mode */
    int stereo;

    struct Node {
      /*! hostname at which to reach this node */
      std::string hostName;
      /*! port at which this node waits for connections */
      int         port;
      /*! region of pixels that this node is responsible for */
      box2i       region; 
    };
    
    /*! the nodes that collectively offer this service. When using a
        dispatcher (ie, "head node" this will be a single node (the
        head node), even though there may be multiple nodes 'behind'
        that head node */
    std::vector<Node> nodes;
    
    /*! read a service info from a given hostName:port. The service
      has to already be running on that port 

      Note this may throw a std::runtime_error if the connection
      cannot be established 
    */
    void getFrom(const std::string &hostName,
                 const int portNo);

    /*! write this data to a socket, in a form that we can afterwards
        parse */
    void writeTo(sock::socket_t socket);

    /*! the magic number we use to make sure that incoming tile
      connections are actually for this wall */
    const size_t magic;
  };

  // struct ControlWindowImageInfo{
  //   typedef std::shared_ptr<ControlWindowImageInfo> SP;
    
  //   struct Node {
  //     /*! hostname at which to reach this node */
  //     std::string hostName;
  //     /*! port at which this node waits for connections */
  //     int         port;
  //   };

  //   const size_t magic;
  //   Node node;

  //   void writeTo(sock::socket_t socket);
  //   static ControlWindowImageInfo::SP getInfo(const std::string hostName,
  //                  const int portNo);
    
  //   ControlWindowImageInfo(const size_t magic = random())
  //     :magic(magic)
  //   {
  //     std::cout << "mamamagic " << magic << std::endl;
  //   }
    
  // };

} // ::dw2
