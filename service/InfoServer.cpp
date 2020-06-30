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

#include "InfoServer.h"

namespace dw2 {
  InfoServer::InfoServer(const int portNumber, 
                         ServiceInfo::SP serviceInfo)
    : serviceInfo(serviceInfo)
  {
    socket = nullptr;
    int nextPortToTry = portNumber;
    
    while (socket == NULL) {
      socket = sock::bind(nextPortToTry);
      if (socket != NULL)
        break;
      nextPortToTry++;
    }
    
    printf("=======================================================\n");
    printf("#dw2.server.info: opened display wall info port on %s:%i\n",
           getHostName().c_str(),nextPortToTry);
    printf("=======================================================\n");
    
    thread = std::thread([this]() { this->serveInfoThreadFunction(); });
  }

  void InfoServer::serveInfoThreadFunction() const 
  {
    while (1) {
      std::cout << "#dw2.server.info: info thread active, waiting for info requests" << "\n";
      std::cout << "info requester " << socket << std::endl;
      sock::socket_t infoRequester = sock::listen(socket);
      std::cout << "#dw2.server.info: got info request, writing info" << "\n";
      
      serviceInfo->writeTo(infoRequester);
      sock::close(infoRequester);
    }
  }

  // ControlWindowServer::ControlWindowServer(const int portNumber, ControlWindowImageInfo::SP controlWindowImageInfo)
  // :controlWindowImageInfo(controlWindowImageInfo)
  // {
  //   socket = nullptr;
  //   int nextPortToTry = portNumber;
    
  //   while (socket == NULL) {
  //     socket = sock::bind(nextPortToTry);
  //     if (socket != NULL)
  //       break;
  //     nextPortToTry++;
  //   }
    
  //   printf("=======================================================\n");
  //   printf("#dw2.server.control-window.info: opened display wall info port on %s:%i\n",
  //          getHostName().c_str(),nextPortToTry);
  //   printf("=======================================================\n");

  //   thread = std::thread([this]() { this->serveInfoThreadFunction(); });
  // }
  
  // void ControlWindowServer::serveInfoThreadFunction() const 
  // {
  //   // while(1)
  //   // {
  //     std::cout << "#dw2.control-window.info: thread active, waiting for receive the frames" << "\n";
  //     sock::socket_t infoRequester = sock::listen(socket);
  //     // std::cout << "info requester " << socket << std::endl;
  //     std::cout << "#dw2.server control-window .info: got info request, writing info" << "\n";
  //     // std::cout << "info requester " << infoRequester << std::endl;
  //     controlWindowImageInfo->writeTo(infoRequester);
  //     sock::close(infoRequester);
  //   // }
  // }


} // ::dw2
