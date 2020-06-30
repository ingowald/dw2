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

#include "../common/mpi_util.h"
#include "../common/ServiceInfo.h"

namespace dw2 {

  // ==================================================================
  /*! the class that will serve wall info requests. This will always
      run on rank 0, which is either the head node (when head node is
      enabled), or display number 0) */
  struct InfoServer {
    typedef std::shared_ptr<InfoServer> SP;
    
    InfoServer(const int portNumber,
               ServiceInfo::SP serviceInfo);

  private:
    /*! function that runs the info server as a separate thread */
    void serveInfoThreadFunction() const;
    
    /*! the socket under which we offer our service */
    sock::socket_t socket { nullptr };

    /*! the thread we use to offer this service */
    std::thread thread;

    /*! the service info we're serving */
    ServiceInfo::SP serviceInfo;
  };

  // // ==================================================================
  // /*! the class that will serve receiving frames for the control window. This will always
  //     run on rank 0, which is either the head node (when head node is
  //     enabled), or display number 0) */
  // struct ControlWindowServer {
  //   typedef std::shared_ptr<ControlWindowServer> SP;
    
  //   ControlWindowServer(const int portNumber, ControlWindowImageInfo::SP controlWindowImageInfo);

  // private:
  //   /*! function that runs the info server as a separate thread */
  //   void serveInfoThreadFunction() const;
    
  //   /*! the socket under which we offer our service */
  //   sock::socket_t socket { nullptr };

  //   /*! the thread we use to offer this service */
  //   std::thread thread;

  //   ControlWindowImageInfo::SP controlWindowImageInfo;

  //   size_t dataSize;

  //   /*! the date pointer*/

  // };
} // ::dw2
