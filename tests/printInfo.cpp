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
#include "../common/vec.h"

namespace dw2 {

  extern "C" int main(int ac, char **av)
  {
    if (ac != 3)
      throw std::runtime_error("usage: ./dw2_info <hostname> <portNum>");

    const std::string hostName = av[1];
    const int portNum = atoi(av[2]);
    std::cout << "Trying to connect to display wall info port at "
              << hostName << ":" << portNum << "\n";
    dw2_info_t info;
    if (dw2_query_info(&info,hostName.c_str(),portNum) != DW2_OK) {
      std::cerr << "could not connect to display wall ..." << "\n";
      exit(1);
    }

    std::cout << "=======================================================" << "\n";
    std::cout << "Found display wall service at " << hostName << ":" << portNum << "\n";
    std::cout << "- total num pixels in wall: " << (vec2i&)info.totalPixelsInWall << "\n";
    std::cout << "=======================================================" << "\n";
    return 0;
  }
    
} // ::dw2
