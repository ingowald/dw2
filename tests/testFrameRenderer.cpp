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
#include "../common/mpi_util.h"
#include <unistd.h>

#include <chrono>

namespace dw2 {

  extern int dbg_rank;
  
  int mpi_rank = -1;
  int mpi_size = -1;

  void usage(const std::string &errMsg, int rank)
  {
    if (rank == 0) {

      if (errMsg != "")
        std::cerr << "Error: " << errMsg << "\n" << "\n";
      
      std::cout << "Usage: ./testFrameRenderer <host> <port> <args>*" << "\n";
    }
    exit(errMsg != "");
  }
  
  extern "C" int main(int ac, char **av)
  {
    MPI_CALL(Init(&ac,&av));
    MPI_CALL(Comm_rank(MPI_COMM_WORLD,&mpi_rank));
    MPI_CALL(Comm_size(MPI_COMM_WORLD,&mpi_size));
    dbg_rank = mpi_rank;

    /*! various options to configure test renderer, so we can stress
        different pieces of the communication pattern */
    int    numFramesToRender = -1;//100;
    size_t usleepPerFrame    = 0;
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
      if (arg == "--barrier-per-frame")
        barrierPerFrame = true;
      else if(arg == "--tile-size")
        s = std::atoi(av[++i]);
      else if (arg[0] == '-')
        usage("un-recognized cmd-line argument '"+arg+"'",mpi_rank);
      else if (hostName == "")
        hostName = arg;
      else if (port == 0)
        port = stoi(arg);
      else
        usage("both hostname and port are already specified ... !?",mpi_rank);
    }

    if (hostName == "")
      usage("no hostname specified...",mpi_rank);
    if (port == 0)
      usage("no port specified...",mpi_rank);

    vec2i  tileSize          = { s,s };
    std::cout << "tilesize = " << s << "\n";
    
    dw2_info_t info;
    if (mpi_rank == 0)
      std::cout << "querying wall info from " << hostName << ":" << port << "\n";
    
    if (dw2_query_info(&info,hostName.c_str(),port) != DW2_OK) {
        if (mpi_rank == 0)
            std::cerr << "could not connect to display wall ..." << "\n";
        exit(1);
    }
    const vec2i wallSize = (const vec2i&)info.totalPixelsInWall;
    const int hasControlWindow = (const int&)info.hasControlWindow;
    const vec2i controlWindowSize = (const vec2i&)info.controlWindowSize;

    std::cout << "DEBUG: wallSize = " << wallSize.x << " " << wallSize.y << "\n";
    if(hasControlWindow)
      std::cout << "Debug: control window size : " << controlWindowSize.x << " " << controlWindowSize.y << "\n";

    // const osp::vec2i wallSize{23040, 5760};
    // const osp::vec2i wallSize{1024, 1024};
        
    dw2_connect(hostName.c_str(),port, mpi_size);

    // if(hasControlWindow && mpi_rank == 0){
    //   //!connect client rank 0 with the service rank 0 throuth port 8443
    //   int p = 8443;
    //   dw2_connect(hostName.c_str(), p, 1);
    // }
    // std::cout << "All connection established !!!!!!!!!!!! " << "\n";
    static double t_last = -1.;
    static double t_beginFrame_total = 0;
    // ==================================================================
    // the actual work
    // ==================================================================

    for (int frameID=0;frameID!=numFramesToRender;frameID++) {
      //std::cout << "node " << mpi_rank << " rendering frame " << frameID << "\n";
      double t_start = getCurrentTime();
      dw2_begin_frame();
      double t_end = getCurrentTime();
      t_beginFrame_total += t_end - t_start;
      // beginFrameWaitTime.push_back(t_end - t_start);
      
      vec2i numTiles = divRoundUp(wallSize,tileSize);
      parallel_for(numTiles.x*numTiles.y,[&](int tileID){
          double ts_write = getCurrentTime();
          int tileOwner
            = (randomizeOwner
               ? ((tileID * 13 * 17 + 0x123ULL) * 11 *3 + 0x4343ULL)
               : tileID)
            % mpi_size;
          if (mpi_rank != tileOwner) return;

          const int tile_x  = tileID % numTiles.x;
          const int tile_y  = tileID / numTiles.x;
          const int begin_x = tile_x * tileSize.x;
          const int begin_y = tile_y * tileSize.y;
          const int end_x   = std::min(begin_x+tileSize.x,wallSize.x);
          const int end_y   = std::min(begin_y+tileSize.y,wallSize.y);

          uint32_t pixel[tileSize.x*tileSize.y];
          
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

          // std::cout << "node " << mpi_rank << " time of writing one tile " << ts_send - ts_write << " s" << "\n";
          // writeTileTime.push_back(ts_send - ts_write);
          dw2_send_rgba(begin_x,begin_y,
                        end_x-begin_x,
                        end_y-begin_y,
                        /* pitch */tileSize.x,
                        pixel);
          double te_send = getCurrentTime();
          // std::cout << "node " << mpi_rank << " time of sending one tile " << te_send - ts_send << " s" << "\n";
          // sendTileTime.push_back(te_send - ts_send);
          // std::cout << "node " << mpi_rank << " time of writing one tile " << ts_send - ts_write << " s"
          //                                  << " time of sending one tile : " << te_send - ts_send << "\n";
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

      if (barrierPerFrame)
        MPI_CALL(Barrier(MPI_COMM_WORLD));
      dw2_end_frame();
      usleep(usleepPerFrame);
    }
    
    MPI_CALL(Barrier(MPI_COMM_WORLD));
    dw2_disconnect();
    MPI_CALL(Barrier(MPI_COMM_WORLD));
    MPI_CALL(Finalize());
  }

}

