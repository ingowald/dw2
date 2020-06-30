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

#include <chrono>

namespace dw2 {

  void usage(const std::string &errMsg)
  {
	  if (errMsg != "")
		  std::cerr << "Error: " << errMsg << "\n" << "\n";

	  std::cout << "Usage: ./testFrameRenderer <host> <port> <args>*" << "\n";
  }
  
  extern "C" int main(int ac, char **av)
  {
    /*! various options to configure test renderer, so we can stress
        different pieces of the communication pattern */
    int    numFramesToRender = -1;//100;
    bool   barrierPerFrame   = false;
    int s = 32;
    
    /*! if randomizeowner is false, each tile gets done by same rank
        every frame with pretty much same numebr of tiles for rank
        every frame; if it is true we will randomlzie this, which may
        also change the number of tiles any rank produces */
    bool        randomizeOwner  = false;
    std::string hostName = "";
    int         port = 0;

    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if(arg == "--tile-size")
        s = std::atoi(av[++i]);
      else if (arg[0] == '-')
        usage("un-recognized cmd-line argument '"+arg+"'");
      else if (hostName == "")
        hostName = arg;
      else if (port == 0)
        port = stoi(arg);
      else
        usage("both hostname and port are already specified ... !?");
    }

    if (hostName == "")
      usage("no hostname specified...");
    if (port == 0)
      usage("no port specified...");

    vec2i  tileSize          = { s,s };
    std::cout << "tilesize = " << s << "\n";
    
    dw2_info_t info;
	std::cout << "querying wall info from " << hostName << ":" << port << "\n";
    
    if (dw2_query_info(&info,hostName.c_str(),port) != DW2_OK) {
		std::cerr << "could not connect to display wall ..." << "\n";
        exit(1);
    }
    const vec2i wallSize = (const vec2i&)info.totalPixelsInWall;
    const int hasControlWindow = (const int&)info.hasControlWindow;
    const vec2i controlWindowSize = (const vec2i&)info.controlWindowSize;

    std::cout << "DEBUG: wallSize = " << wallSize.x << " " << wallSize.y << "\n";
    dw2_connect(hostName.c_str(),port, 1);

    static double t_last = -1.;
    static double t_beginFrame_total = 0;
    // ==================================================================
    // the actual work
    // ==================================================================

    for (int frameID=0;frameID!=numFramesToRender;frameID++) {
      double t_start = getCurrentTime();
      dw2_begin_frame();
      double t_end = getCurrentTime();
      t_beginFrame_total += t_end - t_start;
      // beginFrameWaitTime.push_back(t_end - t_start);
      
      vec2i numTiles = divRoundUp(wallSize,tileSize);
      parallel_for(numTiles.x*numTiles.y,[&](int tileID){
          double ts_write = getCurrentTime();

          const int tile_x  = tileID % numTiles.x;
          const int tile_y  = tileID / numTiles.x;
          const int begin_x = tile_x * tileSize.x;
          const int begin_y = tile_y * tileSize.y;
          const int end_x   = std::min(begin_x+tileSize.x,wallSize.x);
          const int end_y   = std::min(begin_y+tileSize.y,wallSize.y);

		  std::vector<uint32_t> pixel(tileSize.x*tileSize.y, 0);
          
          for (int iy=begin_y;iy<end_y;iy++)
            for (int ix=begin_x;ix<end_x;ix++) {
              int local_x = ix-begin_x;
              int local_y = iy-begin_y;

              int r = ((ix+frameID) % 256);
              int g = ((iy+frameID) % 256);
              int b = ((ix+iy+frameID) % 256);
              int local_idx = local_x+tileSize.x*local_y;
              pixel[local_idx] = (r) | (g<<8) | (b<<16) | (r << 24);
            }
          double ts_send = getCurrentTime();

          dw2_send_rgba(begin_x,begin_y,
                        end_x-begin_x,
                        end_y-begin_y,
                        /* pitch */tileSize.x,
                        pixel.data());
          double te_send = getCurrentTime();
        });
        double t_out = getCurrentTime();
        // std::cout << "#client: avg frame rate: " << (1.f/ (t_out - t_last)) << "fps" << "\n";
        // totalTime.push_back(t_out - t_last);
        if (t_last < 0.) {
            t_last = t_out;
          }
          static double t_avg = 0.f;
          if (t_last >= 0) {
            double t = t_out - t_last;
            if (t_avg == 0.f)
              t_avg = t;
            else
              t_avg = (9.*t_avg + t)/10.;
            // std::cout << "#client: avg frame rate: " << (1.f/t_avg) << "fps" << " begin frame wait time : " << t_beginFrame_total / frameID << "s" << "\n";
          }
        t_last = t_out;

      dw2_end_frame();
    }
    
    dw2_disconnect();
  }

}


